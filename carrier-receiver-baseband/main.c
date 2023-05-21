/**
 * (Original authors)
 * Tobias Mages & Wenqing Yan
 * Backscatter PIO
 * 02-March-2023
 * 
 * (Modified with BCH by Po-Hsuan Chou)
 * 18-May-2023
 *
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"

#include "pico/util/queue.h"
#include "pico/binary_info.h"
#include "pico/util/datetime.h"
#include "hardware/spi.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "backscatter.h"
#include "packet_generation.h"

#define RADIO_SPI             spi0

#define TX_DURATION            200 // send a packet every 50ms
#define PACKET_DURATION        100
#define RECEIVER              1352 // define the receiver board either 2500 or 1352
#define PIN_TX1                  6
#define PIN_TX2                 27
#define CLOCK_DIV0              34 // larger
#define CLOCK_DIV1              32 // smaller
#define DESIRED_BAUD         40000
#define TWOANTENNAS          true

#define CARRIER_FEQ     2450000000

int main() {

    /* setup SPI */
    stdio_init_all();
    spi_init(RADIO_SPI, 5 * 1000000); // SPI0 at 5MHz.

    sleep_ms(3000);

    /* setup backscatter state machine */
    PIO pio = pio0;
    uint sm = 0;
    struct backscatter_config backscatter_conf;
    uint16_t instructionBuffer[32] = {0}; // maximal instruction size: 32
    backscatter_program_init(pio, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS);

    static uint8_t message[PAYLOADSIZE + HEADER_LEN];  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];
    
    // Mini packet: Header (10 bytes) + Payload (2 bytes)
    // Original: [3], BCH: [4].
    static uint32_t bf_0[4] = {0};
    static uint32_t bf_1[4] = {0};
    static uint32_t bf_2[4] = {0};
    static uint32_t bf_3[4] = {0};
    static uint32_t bf_4[4] = {0};
    static uint32_t bf_5[4] = {0};
    static uint32_t bf_6[4] = {0};


    /* loop */
    while (true) {

        uint32_t encode = 0, decode;
        uint32_t res = 0;

        /* generate new data */
        generate_data(tx_payload_buffer, PAYLOADSIZE, true);

        /* add header (10 byte) to packet */
        add_header(&message[0], seq, header_tmplate);

        /* add payload to packet */
        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);


        // Printing for debug
        // buffer_size(PAYLOADSIZE, HEADER_LEN): 6 (*4 = 24)
        // printf("Buffer_size (PAYLOADSIZE, HEADER_LEN): %d\n", buffer_size(PAYLOADSIZE, HEADER_LEN));
        
        // Print packet content in DECIMAL (for debug)
        printf("Packet content in Dec: ");
        for (uint8_t i=0; i < sizeof(message); i++) {
            printf("%d ", message[i]);
        }
        printf("\n");

        /* casting for 32-bit fifo (old)
        printf("FIFO BUFFER (uint32_t):\n");
        for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
            buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            printf("No.%d: %d\n", i, buffer[i]);
        }*/

        for (uint8_t i=0; i < 2; i++) {
            bf_0[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            bf_1[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            bf_2[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            bf_3[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            bf_4[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            bf_5[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            bf_6[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
        }

        bf_0[2] = ((uint32_t) message[11]) | (((uint32_t) message[10]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);
        bf_1[2] = ((uint32_t) message[13]) | (((uint32_t) message[12]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);
        bf_2[2] = ((uint32_t) message[15]) | (((uint32_t) message[14]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);
        bf_3[2] = ((uint32_t) message[17]) | (((uint32_t) message[16]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);
        bf_4[2] = ((uint32_t) message[19]) | (((uint32_t) message[18]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);
        bf_5[2] = ((uint32_t) message[21]) | (((uint32_t) message[20]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);
        bf_6[2] = ((uint32_t) message[23]) | (((uint32_t) message[22]) << 8) | (((uint32_t) message[9]) << 16) | (((uint32_t)message[8]) << 24);

        for(int i = 0; i<3; i++){
            printf("BF0 - %d: %x\n", i, bf_0[i]);
            printf("BF1 - %d: %x\n", i, bf_1[i]);
            printf("BF2 - %d: %x\n", i, bf_2[i]);
            printf("BF3 - %d: %x\n", i, bf_3[i]);
            printf("BF4 - %d: %x\n", i, bf_4[i]);
            printf("BF5 - %d: %x\n", i, bf_5[i]);
            printf("BF6 - %d: %x\n", i, bf_6[i]);
        }

        /* Set BCH for payload */
        setPacket(bf_0);
        setPacket(bf_1);
        setPacket(bf_2);
        setPacket(bf_3);
        setPacket(bf_4);
        setPacket(bf_5);
        setPacket(bf_6);

        
        printf("\n");
                    
        /*put the data to FIFO*/
        // Size of buffer: 24 (bytes)
        // printf("sizeof(buffer): %d\n", sizeof(buffer));
        // printf("\n");
        // Old backscattering
        // backscatter_send(pio,sm, buffer, sizeof(buffer));

        // Old (pio, sm, bf, 3), New (pio, sm, bf, 4);
        backscatter_send(pio,sm, bf_0, 4);
        sleep_ms(PACKET_DURATION);
        backscatter_send(pio,sm, bf_1, 4);
        sleep_ms(PACKET_DURATION);
        backscatter_send(pio,sm, bf_2, 4);
        sleep_ms(PACKET_DURATION);
        backscatter_send(pio,sm, bf_3, 4);
        sleep_ms(PACKET_DURATION);
        backscatter_send(pio,sm, bf_4, 4);
        sleep_ms(PACKET_DURATION);
        backscatter_send(pio,sm, bf_5, 4);
        sleep_ms(PACKET_DURATION);
        backscatter_send(pio,sm, bf_6, 4);

        printf("------------------------------------------------------------------------------------------");
        printf("\n");

        seq++;

        sleep_ms(TX_DURATION);
    }

}
