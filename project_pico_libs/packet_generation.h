/**
 * Tobias Mages & Wenqing Yan
 *
 * support functions to generate payload data and function
 *
 */

#ifndef PACKET_GERNATION_LIB
#define PACKET_GERNATION_LIB

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "packet_generation.h"

#define DEFAULT_SEED 0xABCD

#ifndef USE_ECC
#define USE_ECC 0
#endif

#ifndef USE_FEC
#define USE_FEC 0
#endif

#if USE_RETRANSMISSION == 1
extern uint8_t rtx_enabled;
#endif

#if USE_ECC == 1
#define PAYLOADSIZE 12 * 3 + 2
#elif USE_FEC == 1
#define PAYLOADSIZE 6
#define NUM_CODES 16
#define NUM_SAMPLE_POS_CODES 8
#define DATA_BITS 4
static uint8_t walsh_combinations[NUM_CODES] = {
    0b0000,
    0b0001,
    0b0010,
    0b0011,
    0b0100,
    0b0101,
    0b0110,
    0b0111,
    0b1000,
    0b1001,
    0b1010,
    0b1011,
    0b1100,
    0b1101,
    0b1110,
    0b1111,
};
static uint8_t sample_pos_walsh_combinations[NUM_SAMPLE_POS_CODES] = {
    0b00,
    0b01,
    0b10,
    0b11,
    /* Not used below */
    0b00,
    0b00,
    0b00,
    0b00,
};
extern uint64_t walsh_codes[NUM_CODES];
void init_walsh();
#else
#define PAYLOADSIZE 14
#endif

#define HEADER_LEN 10 // 8 header + length + seq
#define buffer_size(x, y) (((x + y) % 4 == 0) ? ((x + y) / 4) : ((x + y) / 4 + 1)) // define the buffer size with ceil((PAYLOADSIZE+HEADER_LEN)/4)

#ifndef MINMAX
#define MINMAX
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

/*
 * obtain the packet header template for the corresponding radio
 */
uint8_t *packet_hdr_template(uint16_t receiver);

/*
 * generate of a uniform random number.
 */
extern uint32_t seed;
uint32_t rnd();

/*
 * generate compressible payload sample
 * file_position provides the index of the next data byte (increments by 2 each time the function is called)
 */
extern uint16_t file_position;
uint16_t generate_sample();

/*
 * fill packet with 16-bit samples
 * include_index: shall the file index be included at the first two byte?
 * length: the length of the buffer which can be filled with data
 */
#if USE_FEC == 1
extern uint8_t sample_position;
extern uint16_t prev_sample;
#endif
void generate_data(uint8_t *buffer, uint8_t length, bool include_index);

/* including a header to the packet:
 * - 8B header sequence
 * - 1B payload length
 * - 1B sequence number
 *
 * packet: buffer to be updated with the header
 * seq: sequence number of the packet
 */
void add_header(uint8_t *packet, uint8_t seq, uint8_t *header_template);

#endif
