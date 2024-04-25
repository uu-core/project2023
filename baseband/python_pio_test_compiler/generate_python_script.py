import math
import re
from typing import Self, Dict, Callable, Union, List, Final
from pydantic import BaseModel


class EmptyOutBufferException(Exception):
    "Raised when input buffer is empty and an out instruction was requested"
    pass


class PioState(BaseModel):
    # the pio code, without comments and split on newlines
    code: List[str]
    # the output buffer for the pio
    out_buffer: list[int]  # this should only have 1 or 0
    # the x register
    x: int = 0
    # the y register
    y: int = 0
    # how many instructions/cycles have passed
    step: int = 0
    # how long to delay until the next instruction
    current_delay: int = 0
    # the value of the pin controlled by the pio
    internal_pin: bool = False
    # the index of the next instruction to execute
    instruction_index: int = 0
    # the value of any outside pins (used for wait)
    outside_pins: Dict[int, bool] = {}
    # if currently wait for a condition
    is_waiting: bool = False
    # the function to call to get if the wait should stop
    should_continue_waiting: Union[Callable[[], bool], None] = None
    # the list of labels to jump to and the index of the code to jump to
    jump_labels: Dict[str, int]

    # get the value of the outside pin
    def get_outside_pin(self, index: int) -> bool:
        if self.outside_pins.get(index) is None:
            return False
        return self.outside_pins.get(index)

    # set the value of the outside pin
    def set_outside_pin(self, index: int, value: bool) -> None:
        self.outside_pins[index] = value

    # set a delay (used to set delays after instructions)
    def set_delay(self, delay: int) -> None:
        if delay > 32:
            raise ValueError(f"set_delay: {delay} is greater than 32 (disallowed by pio I think)")
        self.current_delay = delay

    # get the next set of bits as an int value, used to load in bits to a register
    def get_input_bits_as_int(self, num_of_bits: int) -> int:
        d: int = 0
        for i in range(num_of_bits):
            d = (d << 1) | self.out_buffer.pop()
        return d

    # jump to a label
    def jump_to(self, label: str) -> None:
        jump_index = self.jump_labels.get(label)
        if jump_index is None:
            raise ValueError(f"jump_to: unknown label {label}")
        self.instruction_index = jump_index

    # get the value of the variable that is the same name as the string provided
    def get_variable_from_str(self, var_str: str) -> int:
        value = {"y": self.y, "x": self.x, }[var_str]
        if value is None:
            raise ValueError(f"get_variable_from_str: unknown variable: {var_str}")
        return value

    # set the value of the variable that is the same name as the string provided
    def set_variable_from_str(self, var_str: str, value: int) -> None:
        if var_str == "y":
            self.y = value
        elif var_str == "x":
            self.x = value
        else:
            raise ValueError(f"set_variable_from_str: unknown variable: {var_str}")

    # generate the jump labels dict from the code
    @classmethod
    def get_labels_dict(cls, code: List[str]) -> Dict[str, int]:
        labels_dict: Dict[str, int] = {}
        for i, line in enumerate(code):
            split = re.split(r"\s+", line.strip())
            if split[0] == ".wrap_target":
                labels_dict[split[0]] = i
            elif split[0][-1] == ":":
                labels_dict[split[0].replace(":", "")] = i
        return labels_dict

    # called on instruction line to add delay if it exists
    def set_delay_if_needed(self, split: List[str]) -> None:
        line = "".join(split)
        s1 = line.strip().split("[")
        if len(s1) > 1:
            delay = int(s1[1].split("]")[0])
            self.set_delay(delay)

    def execute_set_pins_instruction(self, split: List[str]) -> None:
        # SET pins 1 [30] // max 32 i think
        # SET pins 0 [28]
        if split[2] == "0":
            self.internal_pin = False
        elif split[2] == "1":
            self.internal_pin = True
        else:
            raise ValueError(f"execute_set_pins: unrecognized pin setting {split[2]}")

        self.set_delay_if_needed(split)

    def execute_out_instruction(self, split: List[str]) -> None:
        # OUT     y  32
        # OUT  x  1
        # OUT  x  1   [23] # is possible

        bits = int(split[2])
        self.set_variable_from_str(split[1], self.get_input_bits_as_int(bits))
        self.set_delay_if_needed(split)

    def execute_wait_instruction(self, split: List[str]) -> None:
        # WAIT 0 pin 3
        # WAIT 0 pin 3 [10] # possible
        pin_num = int(split[3])
        self.is_waiting = True
        if split[1] == "0":
            self.should_continue_waiting = lambda: self.get_outside_pin(pin_num)
        elif split[1] == "1":
            self.should_continue_waiting = lambda: not self.get_outside_pin(pin_num)
        else:
            raise ValueError(f"execute_wait_instruction: unrecognized value {split[1]}")
        self.set_delay_if_needed(split)

    def execute_jmp_instruction(self, split: List[str]) -> None:
        # JMP get_symbol
        # JMP x-- loop_wait
        #  JMP !x  send_b10   ; jmp if x is zero
        if len(split) == 2:
            self.jump_to(split[1])
        elif len(split) == 3:
            condition = split[1]
            label = split[2]
            if condition[0] == "!":
                if self.get_variable_from_str(condition[1]) > 0:
                    self.jump_to(label)
            elif condition[-1] == "-":
                value_of_cond_var = self.get_variable_from_str(condition[0])
                # todo fix this b/c sometimes on pico if x == 0 will still jump
                if value_of_cond_var > 0:
                    self.set_variable_from_str(condition[0], value_of_cond_var - 1)
                    self.jump_to(label)
            else:
                raise ValueError(f"execute_jmp_instruction: illegal jump {"".join(split)}")
        self.set_delay_if_needed(split)

    def execute_mov_instruction(self, split: List[str]) -> None:
        #   MOV x y
        var_to = split[1]
        var_from = split[2]
        self.set_variable_from_str(var_to, self.get_variable_from_str(var_from))
        self.set_delay_if_needed(split)

    def execute_wrap_instruction(self, _: str) -> None:
        #  .wrap /.wrap_target
        self.jump_to(".wrap_target")

    # command strings  ("JMP", etc...) to the function to call
    commands: Final[Dict[str, Callable[[Self, List[str]], None]]] = {
        "JMP": execute_jmp_instruction,
        "MOV": execute_mov_instruction,
        "OUT": execute_out_instruction,
        "SET": execute_set_pins_instruction,
        "WAIT": execute_wait_instruction,
        ".wrap": execute_wrap_instruction
    }

    # if the given line is a valid command, label or . directive,
    # does not check label or directive other that a ":"  at end or "." at beginning
    def is_valid_line(self, split: List[str]) -> bool:
        if split[0][-1] == ":":
            return True
        if split[0][0] == ".":
            return True
        if self.commands.get(split[0]) is not None:
            return True
        return False

    # process the next cycle, ie exec an instruction or wait or delay
    def process_next_step(self) -> str:
        # TODO implement delay
        # TODO implement wait for wait

        if self.current_delay != 0:
            self.current_delay -= 1
            self.step += 1
            return f"{self.step - 1}: DELAYING"
        if self.is_waiting:
            # TODO: this may be wrong
            self.step += 1
            self.is_waiting = self.should_continue_waiting()
            if self.is_waiting:
                return f"{self.step - 1}: WAITING"

        line = self.code[self.instruction_index]
        split = re.split(r"\s+", line.strip())

        if not self.is_valid_line(split):
            raise ValueError(f"Invalid line of code {"".join(split)}")

        command = split[0]
        if command == "OUT" and len(self.out_buffer) == 0:
            raise EmptyOutBufferException()

        command_func = self.commands.get(command)
        if command_func is None:
            self.instruction_index += 1
            return f"NON INSTRUCTION LINE:{"".join(split)}"
        saved_instruction_index = self.instruction_index
        command_func(self, split)

        self.step += 1

        if command == "JMP" and self.instruction_index != saved_instruction_index:
            if len(split) > 2:
                return f"{self.step - 1}: EXEC JMP to {split[2]}"
            else:
                return f"{self.step - 1}: EXEC JMP to {split[1]}"

        self.instruction_index += 1

        if command == "JMP":
            return f"{self.step - 1}: EXEC NO JMP"
        else:
            return f"{self.step - 1}: EXEC {command}"

        # todo implement jump extra

    @staticmethod
    def number_as_bits(num: int) -> List[int]:
        # pio loads msb first, so we want to pack the array lsb to msb
        bits = []
        for i in range(32):
            bit = (num >> i) & 0b1
            bits.append(bit)
        return bits

    @staticmethod
    def create_out_buffer(data: List[int]) -> List[int]:
        data_list: List[int] = []
        for data in data:
            # we want the last item to be read last therefore
            # we need the first item at the end of the list
            data_list = PioState.number_as_bits(data) + data_list
        return data_list

    # set up the class to run
    @staticmethod
    def start(program: str, out_buffer: List[int]):
        # removes comments, splits by newline
        code = [v[0].strip() for v in [x.split(";") for x in program.split("\n")]]
        state = PioState(out_buffer=out_buffer, jump_labels=PioState.get_labels_dict(code), code=code)

        return state
