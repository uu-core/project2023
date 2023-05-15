/**
 * Tobias Mages & Wenqing Yan
 * Backscatter PIO
 * 02-March-2023
 */

#include "backscatter.pio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "math.h"
#include "packet_generation.h"
#include "pico/stdlib.h"
#include <stdio.h>

#ifndef USE_RETRANSMISSION
#define USE_RETRANSMISSION 1
#endif

#if USE_ECC == 1
#define TX_DURATION 30
#elif USE_FEC == 1
#define TX_DURATION 6
#else
#define TX_DURATION 10
#endif

#if USE_RETRANSMISSION == 1
#define RETRANSMISSION_INTERVAL 127
#endif

#define RECEIVER 1352 // define the receiver board either 2500 or 1352
#define PIN_TX1 6
#define PIN_TX2 27

int main()
{
  PIO pio = pio0;
  uint sm = 0;
#if USE_RETRANSMISSION == 1
  uint16_t saved_file_pos = 0;
  uint8_t saved_seq = 0;
  uint32_t saved_seed = DEFAULT_SEED;
#if USE_FEC ==  1
  uint8_t saved_sample_pos = 0;
  uint16_t saved_prev_sample = 0;
#endif
#endif
  uint offset = pio_add_program(pio, &backscatter_program);
  backscatter_program_init(pio, sm, offset, PIN_TX1, PIN_TX2); // two antenna setup
                                                               // backscatter_program_init(pio, sm, offset, PIN_TX1); // one antenna setup

  static uint8_t message[buffer_size(PAYLOADSIZE + 2, HEADER_LEN) * 4] = {0}; // include 10 header bytes
  static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0};         // initialize the buffer
  static uint8_t seq = 0;
  uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
  uint8_t tx_payload_buffer[PAYLOADSIZE];

#if USE_FEC == 1
  init_walsh();

#if USE_RETRANSMISSION == 1
  /* Get the first sample so that we can set the correct saved_prev_sample
     that is used during retransmission. This is only needed for the first
     iteration of the retransmission loop. */
  saved_prev_sample = generate_sample();
  /* Reset the seed again so that the next call to generate_data uses the
     correct sample. */
  seed = DEFAULT_SEED;
#endif
#endif

  while (true)
  {
#if USE_RETRANSMISSION == 1
    if (seq > 0 && (seq % RETRANSMISSION_INTERVAL) == 0)
    {
      uint8_t tmp_seq = seq;
      uint16_t tmp_file_pos = file_position;
      uint32_t tmp_seed = seed;

      seq = saved_seq;
      file_position = saved_file_pos;
      seed = saved_seed;

      saved_seq = tmp_seq;
      saved_file_pos = tmp_file_pos;
      saved_seed = tmp_seed;

#if USE_FEC == 1
      uint8_t tmp_sample_pos = sample_position;
      uint16_t tmp_prev_sample = prev_sample;

      sample_position = saved_sample_pos;
      prev_sample = saved_prev_sample;

      saved_sample_pos = tmp_sample_pos;
      saved_prev_sample = tmp_prev_sample;
#endif
    }
#endif

    /* generate new data */
    generate_data(tx_payload_buffer, PAYLOADSIZE, true);

    /* add header (10 byte) to packet */
    add_header(&message[0], seq, header_tmplate);
    /* add payload to packet */
    memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

    /* casting for 32-bit fifo */
    for (uint8_t i = 0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++)
    {
      buffer[i] = ((uint32_t)message[4 * i + 3]) |
                  (((uint32_t)message[4 * i + 2]) << 8) |
                  (((uint32_t)message[4 * i + 1]) << 16) |
                  (((uint32_t)message[4 * i]) << 24);
    }
    /* put the data to FIFO */
    backscatter_send(pio, sm, buffer, buffer_size(PAYLOADSIZE, HEADER_LEN));
    seq++;
    sleep_ms(TX_DURATION);
  }
}
