//
// Created by Caden Keese on 5/15/24.
//

#include <malloc.h>
#include "pio_bytecode_generator.h"


namespace pio_bytecode {


    static const uint8_t chip_array[16][16] = {
            {0b11, 0b01, 0b10, 0b01, 0b11, 0b00, 0b00, 0b11, 0b01, 0b01, 0b00, 0b10, 0b00, 0b10, 0b11, 0b10},
            {0b11, 0b10, 0b11, 0b01, 0b10, 0b01, 0b11, 0b00, 0b00, 0b11, 0b01, 0b01, 0b00, 0b10, 0b00, 0b10},
            {0b00, 0b10, 0b11, 0b10, 0b11, 0b01, 0b10, 0b01, 0b11, 0b00, 0b00, 0b11, 0b01, 0b01, 0b00, 0b10},
            {0b00, 0b10, 0b00, 0b10, 0b11, 0b10, 0b11, 0b01, 0b10, 0b01, 0b11, 0b00, 0b00, 0b11, 0b01, 0b01},
            {0b01, 0b01, 0b00, 0b10, 0b00, 0b10, 0b11, 0b10, 0b11, 0b01, 0b10, 0b01, 0b11, 0b00, 0b00, 0b11},
            {0b00, 0b11, 0b01, 0b01, 0b00, 0b10, 0b00, 0b10, 0b11, 0b10, 0b11, 0b01, 0b10, 0b01, 0b11, 0b00},
            {0b11, 0b00, 0b00, 0b11, 0b01, 0b01, 0b00, 0b10, 0b00, 0b10, 0b11, 0b10, 0b11, 0b01, 0b10, 0b01},
            {0b10, 0b01, 0b11, 0b00, 0b00, 0b11, 0b01, 0b01, 0b00, 0b10, 0b00, 0b10, 0b11, 0b10, 0b11, 0b01},
            {0b10, 0b00, 0b11, 0b00, 0b10, 0b01, 0b01, 0b10, 0b00, 0b00, 0b01, 0b11, 0b01, 0b11, 0b10, 0b11},
            {0b10, 0b11, 0b10, 0b00, 0b11, 0b00, 0b10, 0b01, 0b01, 0b10, 0b00, 0b00, 0b01, 0b11, 0b01, 0b11},
            {0b01, 0b11, 0b10, 0b11, 0b10, 0b00, 0b11, 0b00, 0b10, 0b01, 0b01, 0b10, 0b00, 0b00, 0b01, 0b11},
            {0b01, 0b11, 0b01, 0b11, 0b10, 0b11, 0b10, 0b00, 0b11, 0b00, 0b10, 0b01, 0b01, 0b10, 0b00, 0b00},
            {0b00, 0b00, 0b01, 0b11, 0b01, 0b11, 0b10, 0b11, 0b10, 0b00, 0b11, 0b00, 0b10, 0b01, 0b01, 0b10},
            {0b01, 0b10, 0b00, 0b00, 0b01, 0b11, 0b01, 0b11, 0b10, 0b11, 0b10, 0b00, 0b11, 0b00, 0b10, 0b01},
            {0b10, 0b01, 0b01, 0b10, 0b00, 0b00, 0b01, 0b11, 0b01, 0b11, 0b10, 0b11, 0b10, 0b00, 0b11, 0b00},
            {0b11, 0b00, 0b10, 0b01, 0b01, 0b10, 0b00, 0b00, 0b01, 0b11, 0b01, 0b11, 0b10, 0b11, 0b10, 0b00},
    };

    extern "C" {
    extern char __StackLimit, __bss_end__;
    uint32_t getTotalHeap(void) {


        return &__StackLimit - &__bss_end__;
    }

    uint32_t getFreeHeap(void) {
        struct mallinfo m = mallinfo();


        return getTotalHeap() - m.uordblks;
    }


    };

    bool print_heap_space_prefix() {
        static uint32_t last_free_space = getFreeHeap();
        static int32_t total_used = 0;

        uint32_t current_free_space = getFreeHeap();

        int32_t diff;

        if (last_free_space > current_free_space) {
            diff = (int32_t) (last_free_space - current_free_space);
        } else {
            diff = 0 - (int32_t) (current_free_space - last_free_space);
        }
        last_free_space = current_free_space;
        total_used = total_used + diff;
        if (diff != 0) {
            printf("Free:%lu Total:%ld Diff:%d -> ", current_free_space, total_used, diff);
            return true;
        }
        return false;

    }

    void print_heap_space() {
        print_heap_space_prefix();
        printf("\n");
    }

    void flip_bytes(std::string &hex_str) {
        unsigned long len = hex_str.length();
        for (int i = 0; i < len; i += 2) {
            if (i < len && i + 1 < len) {
                std::swap(hex_str[i], hex_str[i + 1]);
            }
        }
    }

    std::vector<uint8_t> data_to_chip_vec(const std::string &hex_str) {
        printf("data_to_chip_vec\n");
        print_heap_space();
        std::vector<uint8_t> chips;
        for (uint8_t hex_char: hex_str) {
            uint8_t hex_value;
            if (hex_char >= '0' && hex_char <= '9') hex_value = hex_char - '0';
            else if (hex_char >= 'a' && hex_char <= 'f') hex_value = hex_char - 'a' + 10;
            else if (hex_char >= 'A' && hex_char <= 'F') hex_value = hex_char - 'A' + 10;
            for (int i = 0; i < 16; ++i) {
                chips.push_back(chip_array[hex_value][i]);
            }
        }
        // add middle for O-QPSK
        std::vector<uint8_t> o_chips = {chips[0]};
        // reduce re-allocations
        // o_chips.reserve(chips.size()*2);
        for (int i = 1; i < chips.size(); ++i) {
            uint8_t prev = chips[i - 1];
            uint8_t current = chips[i];
            uint8_t middle = (prev & 0b01) | (current & 0b10);
            o_chips.push_back(middle);
            o_chips.push_back(current);
        }
        printf("data_to_chip_vec end\n");
        print_heap_space();
        return o_chips;
    }

    std::vector<uint8_t> chips_to_length(const std::vector<uint8_t> &chips, int repeats) {
        printf("chips_to_length\n");
        print_heap_space();

        std::vector<std::pair<uint8_t, uint8_t>> waves; // pair<value, repeat>, 2 bytes
        waves.reserve(1028);

        std::vector<uint8_t> lengths;
        lengths.reserve(1028);

        int wave_value = -1;
        int wave_count = 0;


        for (uint8_t chip: chips) {
            for (int i = 0; i < repeats; ++i) {
                switch (chip) {
                    case 0b00:
                        //0000111111110000
                        waves.emplace_back(0, 4);
                        waves.emplace_back(1, 8);
                        waves.emplace_back(0, 4);
                        break;
                    case 0b01:
                        //0000000011111111
                        waves.emplace_back(0, 8);
                        waves.emplace_back(1, 8);
                        break;
                    case 0b10:
                        // 1111111100000000
                        waves.emplace_back(1, 8);
                        waves.emplace_back(0, 8);
                        break;
                    case 0b11:
                        //1111000000001111
                        waves.emplace_back(1, 4);
                        waves.emplace_back(0, 8);
                        waves.emplace_back(1, 4);
                        break;
                }
            }
            if (print_heap_space_prefix()) {
                printf(" chips_to_length %d\n", waves.size());
            }
            if (waves.size() + repeats > 1028) {
                while (waves.size() > 1) {
                    std::pair<uint8_t, uint8_t> curr_wave = waves.front();
                    waves.erase(waves.begin());
                    if (wave_value == -1) {
                        lengths.push_back(curr_wave.first);
                        wave_value = curr_wave.second;
                    }
                    if (curr_wave.first != wave_value) {
                        wave_value = curr_wave.first;
                        lengths.push_back(wave_count);
                        wave_count = 0;
                    }
                    wave_count += curr_wave.second;

                }
            }
        }
        printf("chips_to_length 1\n");
        print_heap_space();


        while (!waves.empty()) {
            std::pair<uint8_t, uint8_t> curr_wave = waves.front();
            waves.erase(waves.begin());
            if (wave_value == -1) {
                lengths.push_back(curr_wave.first);
                wave_value = curr_wave.second;
            }
            if (curr_wave.first != wave_value) {
                wave_value = curr_wave.first;
                lengths.push_back(wave_count);
                wave_count = 0;
            }
            wave_count += curr_wave.second;

        }
        lengths.push_back(wave_count);
        printf("chips_to_length end\n");
        print_heap_space();
        return lengths;
    }


    void set_next_bit(uint32_t &current_value, std::vector<uint32_t> &output, uint8_t &bit_idx, uint8_t set_bit) {
        current_value |= ((uint32_t) set_bit << (31 - bit_idx));

        if (bit_idx == 31) {
            output.push_back(current_value);
            current_value = 0;
            bit_idx = 0;
        } else {
            bit_idx += 1;
        }

    }


    std::vector<uint32_t> to_bits(std::vector<uint8_t> &lengths) {
        printf("to_bits start\n");
        print_heap_space();
        std::vector<uint32_t> data;
        bool starts_high = lengths[0] == 1;
        lengths.erase(lengths.begin());
        uint32_t current_value = 0;
        uint8_t bit_idx = 0;
        if (starts_high) {
            set_next_bit(current_value, data, bit_idx, 0);
        }
        // the first value is if the wave starts high or low
        for (uint8_t len: lengths) {
            for (int j = 0; j < (len - 4) / 2; ++j) {
                set_next_bit(current_value, data, bit_idx, 1);
            }
            set_next_bit(current_value, data, bit_idx, 0);
        }
        if (bit_idx != 0) {
            data.push_back(current_value);
        }
        printf("to_bits end\n");
        print_heap_space();
        return data;
    }


    std::vector<uint32_t> generate(const std::string &hex_str) {
        print_heap_space();
        std::string flipped_hex_str = hex_str;
        flip_bytes(flipped_hex_str);
        printf("Flipped bytes\n");
        print_heap_space();
        auto lengths = chips_to_length(data_to_chip_vec(flipped_hex_str), 4);
        auto bits = to_bits(lengths);
        printf("generate end\n");
        print_heap_space();
        return bits;
    }

} // pio_bytecode