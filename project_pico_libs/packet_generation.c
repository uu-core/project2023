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

#define DEFAULT_SEED 0xABCD
uint32_t seed = DEFAULT_SEED;

uint8_t packet_hdr_2500[HEADER_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0xd3, 0x91, 0xd3, 0x91, 0x00, 0x00};    // CC2500, the last two byte one for the payload length. and another is seq number
uint8_t packet_hdr_1352[HEADER_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0x93, 0x0b, 0x51, 0xde, 0x00, 0x00};    // CC1352P7, the last two byte one for the payload length. and another is seq number

/*
 * obtain the packet header template for the corresponding radio
 * buffer: array of size HEADER_LEN 
 * receiver: radio number (2500 or 1352)
 */
uint8_t *packet_hdr_template(uint16_t receiver){
    if(receiver == 2500){
        return packet_hdr_2500;
    }else{
        return packet_hdr_1352;
    }
}


/*
Chip translation according to Rhode and Schwarz O-QPSK modulation
should take in a 8-bit value where only the 4 lowest bits are used
*/
uint32_t qpsk_chip_translation(char bit_sequence){
    switch (bit_sequence)
    {
    case 0b0000:
        return 0b11101101100111000011010100100010;
    case 0b0001:
        return 0b11101101100111000011010100100010;
    case 0b0010:
        return 0b00101110110110011100001101010010;
    case 0b0011:
        return 0b00100010111011011001110000110101;
    case 0b0100:
        return 0b01010010001011101101100111000011;
    case 0b0101:
        return 0b00110101001000101110110110011100;
    case 0b0110:
        return 0b11000011010100100010111011011001;
    case 0b0111:
        return 0b10011100001101010010001011101101;
    case 0b1000:
        return 0b10001100100101100000011101111011;
    case 0b1001:
        return 0b10111000110010010110000001110111;
    case 0b1010:
        return 0b01111011100011001001011000000111;
    case 0b1011:
        return 0b01110111101110001100100101100000;
    case 0b1100:
        return 0b00000111011110111000110010010110;
    case 0b1101:
        return 0b01100000011101111011100011001001;
    case 0b1110:
        return 0b10010110000001110111101110001100;
    case 0b1111:
        return 0b11001001011000000111011110111000;
    default:
        printf("ERROR: chip_translation has been used with an invalid bit sequence.");
        return -1;
    }
}


/* 
 * generate of a uniform random number.
 */
uint32_t rnd() {
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
uint16_t generate_sample(){
    if (file_position == 0) {
        seed = DEFAULT_SEED; /* reset seed when exceeding uint16_t max */
    }
    file_position = file_position + 2;
    double two_pi = 2.0 * M_PI;
    double u1, u2;
    u1 = ((double) rnd())/ ((double) 0xFFFFFFFF);
    u2 = ((double) rnd())/((double) 0xFFFFFFFF);
    double tmp = ((double) 0x7FF) * sqrt(-2.0 * log(u1));
    return max(0.0,min(((double) 0x3FFFFF),tmp * cos(two_pi * u2) + ((double) 0x1FFF)));
}

/*
 * fill packet with 16-bit samples
 * include_index: shall the file index be included at the first two byte?
 * length: the length of the buffer which can be filled with data
*/
void generate_data(uint8_t *buffer, uint8_t length, bool include_index) {
    if(length % 2 != 0){
        printf("WARNING: generate_data has been used with an odd length.");
    }

    uint8_t data_start = 0;
    if(include_index){
        //split the file_position into two bytes first the MSB part and second the LSB part
        buffer[0]   = (uint8_t) (file_position >> 8);
        buffer[1] = (uint8_t) (file_position & 0x00FF);
        data_start = 2;
    }
    for (uint8_t i=data_start; i < length; i=i+2) {
        uint16_t sample = generate_sample();
        buffer[i]   = (uint8_t) (sample >> 8);
        buffer[i+1] = (uint8_t) (sample & 0x00FF);
    }
}

void generate_static_data(uint8_t *buffer, uint8_t length, bool include_index) {
    if(length % 2 != 0){
        printf("WARNING: generate_data has been used with an odd length.");
    }
    uint8_t data_start = 0;
    if(include_index){
        //split the file_position into two bytes first the MSB part and second the LSB part
        buffer[0]   = (uint8_t) (0 >> 8);
        buffer[1] = (uint8_t) (0 & 0x00FF);
        data_start = 2;
    }
    for (uint8_t i=data_start; i < length; i=i+2) {
        // change below sample to alter messsage sent
        uint16_t sample = 0b0000000011111111;
        buffer[i]   = (uint8_t) 0b00000000;
        buffer[i+1] = (uint8_t) 0b11111111;
    }
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
void add_header(uint8_t *packet, uint8_t seq, uint8_t *header_template) {
    /* fill in the header sequence*/
    for(int loop = 0; loop < HEADER_LEN-2; loop++) {
        packet[loop] = header_template[loop];
        }
    /* add the payload length*/
    packet[HEADER_LEN-2] = 1 + PAYLOADSIZE; // The packet length is defined as the payload data, excluding the length byte and the optional CRC. (cc2500 data sheet, p. 30)
    /* add the packet as sequence number. */
    packet[HEADER_LEN-1] = seq;
}

