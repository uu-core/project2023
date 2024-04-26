//
// Created by Caden Keese on 4/11/24.
//

#include "chip_helpers.h"

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


/**
 * Merge the next chip sequence as uint32_t into a buffer based on the next 4 bits
 * @param out_buffer the buffer to merge the next chip sequence into
 * @param buffer_start_idx the index to the next unset symbol for the buffer
 * @param seq_half_byte the four bits to convert to a chip sequence
 * @param is_first if this is the first data being added to the buffer
 * @return the next starting index
 */
uint append_oqpsk_chip_seq(uint32_t out_buffer[], uint buffer_start_idx, uint8_t seq_half_byte, bool is_first) {
    if (is_first) {
        out_buffer[buffer_start_idx] = expanded_chips[seq_half_byte][0];
    } else {
        uint32_t last = out_buffer[buffer_start_idx - 1];
        uint32_t next = expanded_chips[seq_half_byte][0];
        // the second bit (first bit of data see chip_helpers.h) is the i of the start of the next chip seq
        uint32_t i = (next >> (32 - 2)) & 0b01;
        // the second to last bit (last bit of data see chip_helpers.h) is the 2 of the start of the last chip seq
        uint32_t q = (last >> 1) & 0b01;
        // set the last bit of last as i
        out_buffer[buffer_start_idx - 1] = last | i;
        // set the first bit of next as q
        out_buffer[buffer_start_idx] = next | (q << 31);
    }

    out_buffer[buffer_start_idx + 1] = expanded_chips[seq_half_byte][1];
    return buffer_start_idx + 2;

}

/**
 *  * convert data buffer to the bits to send to pios
 * @param in_buffer the data to convert, converts low 4 bits then high 4 bits of each byte
 * @param in_length the length of the the data to send;
 * @param out_buffer output buffer, where data to send to pio is stored, should be at least 2*in_length*32, (2 symbols per byte, 64 bits per symbol, ie 2 uint32_t)
 * @param out_buffer_idx the start index to save data in the buffer.
 * @return the next open index in the out_buffer, ie last used index +1, so you can repeatedly call this on a buffer if needed;
 */
uint data_to_pio_input(const uint8_t in_buffer[], uint in_length, uint32_t out_buffer[], uint out_buffer_idx) {
    for (uint i = 0; i < in_length; ++i) {
        
        uint8_t i_byte = in_buffer[i];
        uint8_t low = i_byte & 0x0F;
        uint8_t high = i_byte >> 4;
        out_buffer_idx = append_oqpsk_chip_seq(out_buffer, out_buffer_idx, low, i == 0);
        out_buffer_idx = append_oqpsk_chip_seq(out_buffer, out_buffer_idx, high, false);
    }
    return out_buffer_idx;
}




