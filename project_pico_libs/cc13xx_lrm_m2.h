#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

#define K 7
#define DSSS 2



void conv_encoder(bool in, bool* a0, bool* a1);

void reset_conv_reg();

void dsss(bool in, char mode, bool* out);

void encode_payload(uint8_t* in, uint8_t* out, int payloadsize);
void generate_pn9_chip(bool restart);
void whiten_data(uint8_t* white_i, uint8_t* white_o, uint16_t payload_size);