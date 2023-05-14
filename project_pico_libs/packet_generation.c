/**
 * Tobias Mages & Wenqing Yan
 *
 * support functions to generate payload data and function
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "packet_generation.h"
#include "walsh.h"

#define DEFAULT_SEED 0xABCD
uint32_t seed = DEFAULT_SEED;

#if USE_FEC == 1
uint64_t walsh_codes[NUM_CODES];
#endif

uint8_t packet_hdr_2500[HEADER_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0xd3, 0x91, 0xd3, 0x91, 0x00, 0x00}; // CC2500, the last two byte one for the payload length. and another is seq number
uint8_t packet_hdr_1352[HEADER_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0x93, 0x0b, 0x51, 0xde, 0x00, 0x00}; // CC1352P7, the last two byte one for the payload length. and another is seq number

/*
 * obtain the packet header template for the corresponding radio
 * buffer: array of size HEADER_LEN
 * receiver: radio number (2500 or 1352)
 */
uint8_t *packet_hdr_template(uint16_t receiver)
{
    if (receiver == 2500)
    {
        return packet_hdr_2500;
    }
    else
    {
        return packet_hdr_1352;
    }
}

/*
 * generate of a uniform random number.
 */
uint32_t rnd()
{
    const uint32_t A1 = 1664525;
    const uint32_t C1 = 1013904223;
    const uint32_t RAND_MAX1 = 0xFFFFFFFF;
    seed = ((seed * A1 + C1) & RAND_MAX1);
    return seed;
}

/*
 * generate compressible payload sample
 * file_position provides the index of the next data byte (increments by 2 each time the function is called)
 */
uint16_t file_position = 0;
uint16_t generate_sample()
{
    if (file_position == 0)
    {
        seed = DEFAULT_SEED; /* reset seed when exceeding uint16_t max */
    }
    file_position = file_position + 2;
    double two_pi = 2.0 * M_PI;
    double u1, u2;
    u1 = ((double)rnd()) / ((double)0xFFFFFFFF);
    u2 = ((double)rnd()) / ((double)0xFFFFFFFF);
    double tmp = ((double)0x7FF) * sqrt(-2.0 * log(u1));
    return max(0.0, min(((double)0x3FFFFF), tmp * cos(two_pi * u2) + ((double)0x1FFF)));
}

/*
 * fill packet with 16-bit samples
 * include_index: shall the file index be included at the first two byte?
 * length: the length of the buffer which can be filled with data
 */
#if USE_FEC == 1
/* Only 4 bits of data is transmitted in each packet, which means that we
   should only generate a new sample and update the file position every
   4th call of generate_data(). Since the sequence number is increased with
   every packet, we will be able to reconstruct the entire 16-bit sample
   on the receiving end using both the file position and sequence. */
static uint16_t prev_sample;
static uint8_t sample_position = 0;
#endif
void generate_data(uint8_t *buffer, uint8_t length, bool include_index)
{
    if (length % 2 != 0)
    {
        printf("WARNING: generate_data has been used with an odd length.");
    }

    if (sample_position > 3) {
        sample_position = 0;
    }

    uint8_t data_start = 0;
    if (include_index)
    {
        buffer[0] = (uint8_t)(file_position >> 8);
        buffer[1] = (uint8_t)(file_position & 0x00FF);
#if USE_FEC == 1
        // TODO: Repeat sample position bits (2 bits) 4 times to make
        // use of all available bits.
        buffer[2] = sample_position;
        data_start = 3;
#else
        data_start = 2;
#endif
    }

#if USE_ECC == 1
    for (uint8_t i = data_start; i < length; i = i + 6) {
        uint16_t sample = generate_sample();
        uint64_t extended = 0;

        // stutter code SECC (3,1)
        for (int i = 0; i < 16; i++)
        {
            uint8_t bit = (sample & (1 << i)) >> i; // get the i:th bit of sample
            for (uint8_t j = 0; j < 3; j++)
            {
                extended |= (bit << (47 - ((i * 3) + j)));
            }
        }

        memcpy(&buffer[i], &extended, 6);
    }
#elif USE_FEC == 1
    for (uint8_t i = data_start; i < length; i = i + 2)
    {
        uint16_t sample = prev_sample;
        /* It takes 4 calls to this function to fully transmit the entire
           16-bit sample. */
        if (sample_position == 0) {
            sample = generate_sample();
        }

        uint16_t code = 0;
        /* Get the 4-bits of data based on the current sample position. */
        uint8_t data = sample >> (((3 - sample_position) * DATA_BITS) & 0x0F );
        for (int j = 0; j < walsh_combinations; j++)
        {
            if (data == (walsh_combinations[j] & 0x0F))
            {
                code = walsh_codes[j];
                break;
            }
        }

        buffer[i] = (uint8_t)(code >> 8);
        buffer[i + 1] = (uint8_t)(code & 0x00FF);
        sample_position++;
    }
#else
    for (uint8_t i = data_start; i < length; i = i + 6)
    {
        uint16_t sample = generate_sample();
        buffer[i] = (uint8_t)(sample >> 8);
        buffer[i + 1] = (uint8_t)(sample & 0x00FF);
    }
#endif
}

/* including a header to the packet:
 * - 8B header sequence
 * - 1B payload length
 * - 1B sequence number
 *
 * packet: buffer to be updated with the header
 * seq: sequence number of the packet
 * header_template: obtained using packet_hdr_template()
 */
void add_header(uint8_t *packet, uint8_t seq, uint8_t *header_template)
{
    /* fill in the header sequence*/
    for (int loop = 0; loop < HEADER_LEN - 2; loop++)
    {
        packet[loop] = header_template[loop];
    }
    /* add the payload length*/
    packet[HEADER_LEN - 2] = 1 + PAYLOADSIZE; // The packet length is defined as the payload data, excluding the length byte and the optional CRC. (cc2500 data sheet, p. 30)
    /* add the packet as sequence number. */
    packet[HEADER_LEN - 1] = seq;
}

#if USE_FEC == 1
void init_walsh()
{
    /* prepare the walsh codes for use */
    get_walsh_codes(NUM_CODES, walsh_codes[NUM_CODES]);
}
#endif