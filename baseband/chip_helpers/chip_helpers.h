//
// Created by Caden Keese on 4/11/24.
//

#ifndef PICO_EXAMPLES_CHIP_HELPERS_H
#define PICO_EXAMPLES_CHIP_HELPERS_H


#include <pico/types.h>


/**
 *  * convert data buffer to the bits to send to pios
 * @param in_buffer the data to convert, converts low 4 bits then high 4 bits of each byte
 * @param in_length the length of the the data to send;
 * @param out_buffer output buffer, where data to send to pio is stored, should be at least 2*in_length*32, (2 symbols per byte, 64 bits per symbol, ie 2 uint32_t)
 * @param out_buffer_idx the start index to save data in the buffer.
 * @return the next open index in the out_buffer, ie last used index +1, so you can repeatedly call this on a buffer if needed;
 */
uint data_to_pio_input(const uint8_t in_buffer[], uint in_length, uint32_t out_buffer[], uint out_buffer_idx);

// the chip codes expanded to fit the O-QPSK
// [x][0] is the first 31 bits,  the first bit is padded as 0
// [x][1] is the seconds 31 bits, the last bit is padded as 0
const uint32_t  expanded_chips[16][2] = {
        0b01101011110000111110100000010110, // [0][0]
        0b10101010100101000001010101111100, // [0][1]
        0b01111101011010111100001111101000, // [1][0]
        0b00010110101010101001010000010100, // [1][1]
        0b00010101011111010110101111000011, // [2][0]
        0b11101000000101101010101010010100, // [2][1]
        0b00010100000101010111110101101011, // [3][0]
        0b11000011111010000001011010101010, // [3][1]
        0b00101010100101000001010101111101, // [4][0]
        0b01101011110000111110100000010110, // [4][1]
        0b00010110101010101001010000010101, // [5][0]
        0b01111101011010111100001111101000, // [5][1]
        0b01101000000101101010101010010100, // [6][0]
        0b00010101011111010110101111000010, // [6][1]
        0b01000011111010000001011010101010, // [7][0]
        0b10010100000101010111110101101010, // [7][1]
        0b01000001011010010100001010111100, // [8][0]
        0b00000000001111101011111111010110, // [8][1]
        0b01010111110000010110100101000010, // [9][0]
        0b10111100000000000011111010111110, // [9][1]
        0b00111111110101111100000101101001, // [10][0]
        0b01000010101111000000000000111110, // [10][1]
        0b00111110101111111101011111000001, // [11][0]
        0b01101001010000101011110000000000, // [11][1]
        0b00000000001111101011111111010111, // [12][0]
        0b11000001011010010100001010111100, // [12][1]
        0b00111100000000000011111010111111, // [13][0]
        0b11010111110000010110100101000010, // [13][1]
        0b01000010101111000000000000111110, // [14][0]
        0b10111111110101111100000101101000, // [14][1]
        0b01101001010000101011110000000000, // [15][0]
        0b00111110101111111101011111000000, // [15][1]
};



#endif //PICO_EXAMPLES_CHIP_HELPERS_H