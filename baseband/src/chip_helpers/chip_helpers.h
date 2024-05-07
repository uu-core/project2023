//
// Created by Caden Keese on 4/11/24.
//

#ifndef PICO_EXAMPLES_CHIP_HELPERS_H
#define PICO_EXAMPLES_CHIP_HELPERS_H


//#include <pico/types.h>
#include <stdint.h>


/**
 *  * convert data buffer to the bits to send to pios
 * @param in_buffer the data to convert, converts low 4 bits then high 4 bits of each byte
 * @param in_length the length of the the data to send;
 * @param out_buffer output buffer, where data to send to pio is stored, should be at least 2*in_length*32, (2 symbols per byte, 64 bits per symbol, ie 2 uint32_t)
 * @param out_buffer_idx the start index to save data in the buffer.
 * @return the next open index in the out_buffer, ie last used index +1, so you can repeatedly call this on a buffer if needed;
 */
uint32_t data_to_pio_input(const uint8_t in_buffer[], uint32_t in_length, uint32_t out_buffer[], uint32_t out_buffer_idx);


#endif //PICO_EXAMPLES_CHIP_HELPERS_H