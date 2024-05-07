//
// Created by Caden Keese on 4/26/24.
//

#ifndef PIO_GENERATE__DATA_FUNCS_SIGNAL_GENERATE_HELPERS_H
#define PIO_GENERATE__DATA_FUNCS_SIGNAL_GENERATE_HELPERS_H

#include <stdio.h>
#include <stdint.h>

// debug print functions

// from the LSB/ right most byte == 0
#define signal_byte_of(SRC, BYTE_IDX) \
    ((uint8_t) (SRC >> (8*BYTE_IDX)) & 0xFF)


#define SIGNAL_DEBUG
//#define SIGNAL_DEBUG_NO_PRINT_LINE

#ifndef SIGNAL_DEBUG
#define signal_dprintln(FORMAT, ...) ((void)0)
#define signal_dprint(FORMAT, ...) ((void)0)
#define signal_dputs(MSG) ((void)0)
#define signal_dputsln(MSG) ((void)0)
#define signal_bin(MSG, BYTE, BITS) ((void)0)
#define signal_binln(MSG, BYTE, BITS) ((void)0)
#define signal_dprint_int_arr(MSG, ARRAY, ARRAY_LEN) ((void)0)
#define signal_dprint_bytes_arr(MSG, NUM_BYTES, BYTE_SEP, INCLUDE_INDEXES, ARRAY, ARRAY_LEN) ((void)0)
#define signal_bin_bytes(MSG, BYTE_SEP, BYTES, BYTES_LEN, BITS) ((void)0)
#else // SIGNAL_DEBUG
#define SIGNAL_DEBUG_BYTE_TO_BINARY_PATTERN \
    "%c%c%c%c%c%c%c%c"

#define SIGNAL_DEBUG_BYTE_TO_BINARY_BITS(byte, bits)  \
      ((bits==8)?((byte) & 0x80 ? '1' : '0') : '_'), \
      ((bits>=7)?((byte) & 0x40 ? '1' : '0') : '_'), \
      ((bits>=6)?((byte) & 0x20 ? '1' : '0') : '_'), \
      ((bits>=5)?((byte) & 0x10 ? '1' : '0') : '_'), \
      ((bits>=4)?((byte) & 0x08 ? '1' : '0') : '_'), \
      ((bits>=3)?((byte) & 0x04 ? '1' : '0') : '_'), \
      ((bits>=2)?((byte) & 0x02 ? '1' : '0') : '_'), \
      ((bits>=1)?((byte) & 0x01 ? '1' : '0') : '_')
#define signal_dprint(FORMAT, ...) \
    (printf(FORMAT, __VA_ARGS__))

#define signal_dprint_func(FORMAT, ...)\
    signal_dprint("%s(): " FORMAT, __func__, __VA_ARGS__)

#define signal_dprint_func_line(FORMAT, ...) \
    signal_dprint("%s() in %s, line %i: \n" FORMAT, \
        __func__, __FILE__, __LINE__, __VA_ARGS__)

#ifdef SIGNAL_DEBUG_NO_PRINT_LINE
#define signal_dprintln(FORMAT, ...) \
    signal_dprint_func(FORMAT "\n", __VA_ARGS__)
#else // SIGNAL_DEBUG_NO_PRINT_LINE
#define signal_dprintln(FORMAT, ...) \
    signal_dprint_func_line(FORMAT "\n", __VA_ARGS__)
#endif // SIGNAL_DEBUG_NO_PRINT_LINE
#define signal_bin(MSG, BYTE, BITS) \
    signal_dprint("%s"SIGNAL_DEBUG_BYTE_TO_BINARY_PATTERN, MSG, SIGNAL_DEBUG_BYTE_TO_BINARY_BITS(BYTE, BITS))
#define _signal_calc_bits(SUB_BITS, BITS)\
    ((BITS - SUB_BITS) > 0 ?(((BITS - SUB_BITS)>8) ? 8 :(BITS - SUB_BITS)) : 0)


#define signal_dputsln(MSG) \
    signal_dprintln("%s", MSG)
#define signal_dputs(MSG) \
    signal_dprint("%s", MSG)
#define signal_binln(MSG, BYTE, BITS) \
    (signal_bin(MSG, BYTE, BITS),                     \
    signal_dputs("\n"))
#define signal_dprint_int_arr(MSG, ARRAY, ARRAY_LEN) \
    ({\
        signal_dprint_func_line("%s[", MSG);\
        for(int i = 0; i < ARRAY_LEN; i++) {\
            signal_dprint("%d", ARRAY[i]);\
            if(i != ARRAY_LEN-1){\
            signal_dputs(",");\
            }\
        }\
        signal_dputs("]");\
        signal_dputs("\n");\
    })

#define signal_bin_bytes_num(MSG, BYTE_SEP, BYTES, BYTES_LEN, BITS)                                                            \
    ({                                                                                                           \
        signal_dputs(MSG);                                                                                       \
        if (BYTES_LEN>=4) {                                                                                      \
            signal_bin("", signal_byte_of(BYTES, 3), _signal_calc_bits(24,BITS)); \
        }                                                                                                         \
        if (BYTES_LEN>=3) {                                                                                       \
        signal_bin(BYTE_SEP, signal_byte_of(BYTES, 2), _signal_calc_bits(16,BITS));     \
        }                                                                                                       \
        if (BYTES_LEN>=2) {                                                                                     \
            signal_bin(BYTE_SEP, signal_byte_of(BYTES, 1), _signal_calc_bits(8,BITS));   \
        }                                                                                 \
        signal_binln(BYTE_SEP, signal_byte_of(BYTES, 0), _signal_calc_bits(0,BITS));                                          \
        signal_dputs(MSG);                                                                                       \
        if (BYTES_LEN>=4) {                                                                                                   \
            signal_dputs("87654321"BYTE_SEP);\
        }                                                                                                         \
        if (BYTES_LEN>=3) {                                                                                       \
            signal_dputs("87654321"BYTE_SEP);                                                                                        \
        }                                                                                                       \
        if (BYTES_LEN>=2) {                                                                                     \
            signal_dputs("87654321"BYTE_SEP);                                                                                         \
        }                                                                                 \
        signal_dputs("87654321\n");                                    \
    })
#define signal_bin_bytes(MSG, BYTE_SEP, BYTES, BYTES_LEN, BITS)                                                            \
    ({                                                                                                           \
        signal_dputs(MSG);                                                                                       \
        if (BYTES_LEN>=4) {                                                                                      \
            signal_bin("", signal_byte_of(BYTES, 3), _signal_calc_bits(24,BITS)); \
        }                                                                                                         \
        if (BYTES_LEN>=3) {                                                                                       \
        signal_bin(BYTE_SEP, signal_byte_of(BYTES, 2), _signal_calc_bits(16,BITS));     \
        }                                                                                                       \
        if (BYTES_LEN>=2) {                                                                                     \
            signal_bin(BYTE_SEP, signal_byte_of(BYTES, 1), _signal_calc_bits(8,BITS));   \
        }                                                                                 \
        signal_binln(BYTE_SEP, signal_byte_of(BYTES, 0), _signal_calc_bits(0,BITS));                                    \
    })
#define signal_dprint_bytes_arr(MSG, NUM_BYTES,BYTE_SEP,INCLUDE_INDEXES, ARRAY, ARRAY_LEN) \
    ({\
        signal_dprint_func_line("%s[\n", MSG);\
        for(int i = 0; i < ARRAY_LEN; i++) {                                       \
            if(INCLUDE_INDEXES){                                                                       \
                signal_dprint("%d -> ", i);                                                \
            }                                                                       \
            signal_bin_bytes("",BYTE_SEP, ARRAY[i], NUM_BYTES, NUM_BYTES*8);\
        }\
        signal_dputs("\n]\n");\
    })
#endif // SIGNAL_DEBUG

enum SIGNAL_LEVEL {
    SIGNAL_LEVEL_LOW = 0b0,
    SIGNAL_LEVEL_HIGH = 0b1,

};

#define _signal_calc_len_for_convert_to_wave_forms(INPUT_LEN, REPEATS) (INPUT_LEN * 4 * REPEATS)
#define _signal_calc_len_for_convert_waves_to_lengths(INPUT_LEN) ((INPUT_LEN * 2) + 2)
#define _signal_calc_len_for_convert_lengths_to_pio_ints(INPUT_LEN) (((7 * (INPUT_LEN-1))/32)+1)
#define signal_calc_len_for_signal_code(INPUT_LEN, REPEATS)                       \
    _signal_calc_len_for_convert_lengths_to_pio_ints(                    \
        _signal_calc_len_for_convert_waves_to_lengths(                      \
            _signal_calc_len_for_convert_to_wave_forms(INPUT_LEN, REPEATS)       \
        )                                                                 \
    )

/**
 * Convert the signal data to the control bit for the pio
 * @param input_data the data to convert
 * @param input_length the length of the data
 * @param repeats number of times each waveform should repeat
 * @param output_buffer where the output will be saved
 * @param output_length the length of the output buffer, use signal_calc_len_for_signal_code to calculate
 * @return the length of the filled buffer
 */
int convert_to_signal_code(const uint32_t input_data[], uint32_t input_length, int repeats, uint32_t output_buffer[],
                           uint32_t output_length);

#endif //PIO_GENERATE__DATA_FUNCS_SIGNAL_GENERATE_HELPERS_H
