//
// Created by Caden Keese on 4/11/24.
//

#include "chip_helpers.h"


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




