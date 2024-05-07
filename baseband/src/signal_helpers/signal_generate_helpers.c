//
// Created by Caden Keese on 4/26/24.
//

#include <stdbool.h>
#include "signal_generate_helpers.h"


/**

 DONE: make repeats, right now it only does one cycle per phase~~
 TODO: change input to uint32_t from uint8_t


 to enable debug output uncomment below in .h file
 //#define SIGNAL_DEBUG



 call like this:

 #define INPUT_SIZE 3
uint8_t input[INPUT_SIZE] = {
        0b01010101,
        0b01010101,
        0b01010101,
};
 #define output_length  signal_calc_len_for_signal_code(INPUT_SIZE)
 uint32_t output[output_length]
 int output_end = convert_to_signal_code(input, INPUT_SIZE * 3, output, output_length);







 */








const uint16_t waveforms[4] = {
        0b0000000011111111,
        0b0000111111110000,
        0b1111000000001111,
        0b1111111100000000,
};


/**
 * use [_signal_calc_len_for_convert_to_wave_forms] to get size
 * @param input_data  bytes of data, reads each byte MS 2 bits to LS 2 Bits
 * @param output_buffer array that is 4 times input data 2 bits == 1 uint16_t -> 1 byte = 4 uint16_t
 */
uint16_t *
convert_to_wave_forms(const uint32_t input_data[], const uint32_t input_length, const int repeats, uint16_t *output_buffer) {
    int out_index = 0;
    for (int i = 0; i < input_length; ++i) {
        uint32_t i_byte = input_data[i];
        for (int j = 15; j >= 0; --j) {
            uint8_t bits2 = (i_byte >> (j * 2)) & 0b11;
            for (int k = 0; k < repeats; k++) {
                output_buffer[out_index++] = waveforms[bits2];
            }

        }
    }
    return output_buffer;

}


/**
 * convert the waves to lengths of highs and lows
 * @param input_data array of waves
 * @param input_length length of input array
 * @param output_buffer where the output will go, must be (input_length * 2) + 2
 * @param output_buffer_size where the output will go, must be (input_length * 2) + 2
 * @return the length of the data written to [output_buffer] maybe shorter than [output_buffer_size]
 */
uint32_t convert_waves_to_lengths(const uint16_t input_data[],
                             const uint32_t input_length, int output_buffer[],
                             const uint32_t output_buffer_size) {


    int output_index = 0;
    uint32_t lengths_size = (input_length * 2) + 2; // max possible size
    if (output_buffer_size < lengths_size) {
        signal_dprintln("!!! output_buffer_size: %d, is less than needed length: %d!", output_buffer_size,
                        lengths_size);
        return -1;
    }

    int count = 0;
    int high_or_low = -1; // either 1 or 0 to mark high or low, -1 is mark the start
    for (uint32_t i = 0; i < input_length; ++i) {
        uint16_t i_2bytes = input_data[i];
        for (int j = 0; j < 16; j += 4) {
            int index = (16 - j) - 4;
            uint8_t bits4 = (i_2bytes >> index) & 0b1111;
            bool bits_are_high = bits4 > 0;
            if (output_index == 0) {
                if (bits_are_high) {
                    output_buffer[output_index++] = SIGNAL_LEVEL_HIGH; // mark that it starts with a high cycle
                } else {
                    output_buffer[output_index++] = SIGNAL_LEVEL_LOW; // mark that it starts with a low cycle
                }

            }
            if (bits_are_high) {
                // signal has switched
                if (high_or_low != SIGNAL_LEVEL_HIGH) {
                    // set the current level
                    high_or_low = SIGNAL_LEVEL_HIGH;
                    // if there is a last count
                    if (count > 0) {
                        //save the count in the next spot in the array
                        output_buffer[output_index++] = count;
                        // reset count
                        count = 0;
                    }
                }
                count += 4;
            } else {
                // signal has switched
                if (high_or_low != SIGNAL_LEVEL_LOW) {
                    // set the current level
                    high_or_low = SIGNAL_LEVEL_LOW;
                    // if there is a last count
                    if (count > 0) {
                        //save the count in the next spot in the array
                        output_buffer[output_index++] = count;
                        // reset count
                        count = 0;
                    }
                }
                count += 4;
            }
        }
    }
    if (count > 0) {
        //save the count in the next spot in the array
        output_buffer[output_index++] = count;
    }
    signal_dprintln("lengths_index:%d, lengths_size:%d", output_index, lengths_size);

    signal_dprint_int_arr("LENGTHS: ", output_buffer, output_index);


    return output_index;
}


void set_next_bit(uint32_t output_buffer[], int *output_buffer_idx, int *bit_idx, enum SIGNAL_LEVEL level) {

    output_buffer[*output_buffer_idx] |= (level << (31 - *bit_idx));

    if (*bit_idx == 31) {
        *output_buffer_idx += 1;
        // clear any junk data
        output_buffer[*output_buffer_idx] = 0;
        *bit_idx = 0;
        return;
    }
    *bit_idx += 1;


}

int convert_lengths_to_pio_ints(const int input_data[], const uint32_t input_length,
                                uint32_t output_buffer[]) {

    bool is_first_high = input_data[0] == 1;


    int bit_idx = 0;
    int output_buffer_idx = 0;
    if (is_first_high) {
        // the pio starts low so make only do 4 low at first
        // then start the high based on the first length value
        output_buffer[0] = 0;
        bit_idx += 1;
    }
    signal_bin_bytes("\t", ".", output_buffer[0], 4, 32);

    for (int input_index = 1; input_index < input_length; ++input_index) {
        // see pdf working this out on whiteboard
        int duration = input_data[input_index];
        int num_of_1s = (duration / 2) - 2;
        signal_dprint("duration is %d, num_of_1s:%d, output_buffer_idx:%d  -> \n", duration, num_of_1s,
                      output_buffer_idx);
        for (int i = 0; i < num_of_1s; ++i) {
            signal_dputs("\t 1\n");
            set_next_bit(output_buffer, &output_buffer_idx, &bit_idx, SIGNAL_LEVEL_HIGH);
            signal_bin_bytes("\t", ".", output_buffer[output_buffer_idx], 4, 32);
        }
        signal_dputs("\t 0\n");
        set_next_bit(output_buffer, &output_buffer_idx, &bit_idx, SIGNAL_LEVEL_LOW);
        signal_bin_bytes("\t", ".", output_buffer[output_buffer_idx], 4, 32);

    }

    return output_buffer_idx + 1;

}


int
convert_to_signal_code(const uint32_t input_data[], uint32_t input_length, int repeats, uint32_t output_buffer[],
                       uint32_t output_length) {
    uint32_t waves_length = input_length * 3;
    uint16_t waves_array[waves_length];
    // IDK why but unless you pass a pointer back you can no longer access
    uint16_t *waves_ptr = convert_to_wave_forms(input_data, input_length, repeats, waves_array);
    signal_dprint_bytes_arr("waves_ptr->", 2, ".", true, waves_ptr, waves_length);

    uint32_t lengths_length = (waves_length * 2) + 2;
    int lengths_array[lengths_length];
    uint32_t size_of_lengths_array = convert_waves_to_lengths(waves_ptr, waves_length, lengths_array, lengths_length);
    signal_dprint_int_arr("lengths_array->", lengths_array, size_of_lengths_array);
    signal_dprintln("output_length:%d", output_length);
    int ret = convert_lengths_to_pio_ints(lengths_array, size_of_lengths_array, output_buffer);
    return ret;
}

















