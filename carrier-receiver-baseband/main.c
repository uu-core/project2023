/**
 * Tobias Mages & Wenqing Yan
 * Backscatter PIO
 * 02-March-2023
 *
 * See the sub-projects ... for further information:
 *  - baseband
 *  - carrier-CC2500
 *  - receiver-CC2500
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
#include "backscatter.pio.h"
#include "carrier_CC2500.h"
#include "receiver_CC2500.h"
#include "packet_generation.h"

// Carrier GPIO define
#define RADIO_SPI             spi0
#define RADIO_MISO              16
#define RADIO_MOSI              19
#define RADIO_SCK               18

// Missing part from receiver
/* 
 * The following macros are defined in the generated PIO header file 
 * We define them here manually here since this example does not require a PIO state machine.
*/
#define PIO_BAUDRATE 100000
#define PIO_CENTER_OFFSET 6597222
#define PIO_DEVIATION 347222
#define PIO_MIN_RX_BW 794444

// Baseband setting
#define TX_DURATION           50 // send a packet every 50ms
#define RECEIVER              1352 // define the receiver board either 2500 or 1352
#define PIN_TX1                  6
#define PIN_TX2                 27

// Carrier frequency setting @ 2450 Mhz
#define CARRIER_FEQ     2450000000

int main() {
    /* setup SPI */
    stdio_init_all();
    spi_init(RADIO_SPI, 5 * 1000000); // SPI0 at 5MHz.
    gpio_set_function(RADIO_SCK, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MISO, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(RADIO_MOSI, RADIO_MISO, RADIO_SCK, GPIO_FUNC_SPI));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(RX_CSN);
    gpio_set_dir(RX_CSN, GPIO_OUT);
    gpio_put(RX_CSN, 1);
    bi_decl(bi_1pin_with_name(RX_CSN, "SPI Receiver CS"));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(CARRIER_CSN);
    gpio_set_dir(CARRIER_CSN, GPIO_OUT);
    gpio_put(CARRIER_CSN, 1);
    bi_decl(bi_1pin_with_name(CARRIER_CSN, "SPI Carrier CS"));

    /* setup backscatter state machine */
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &backscatter_program);

    //backscatter_program_init(pio, sm, offset, PIN_TX1);
    // Original cmd
    backscatter_program_init(pio, sm, offset, PIN_TX1, PIN_TX2);

    static uint8_t message[PAYLOADSIZE + HEADER_LEN];  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];

    sleep_ms(5000);

    /* Start carrier */
    setupCarrier();
    set_frecuency_tx(CARRIER_FEQ);
    sleep_ms(1);
    startCarrier();
    printf("started carrier\n");

    /* Start Receiver */
    event_t evt = no_evt;
    Packet_status status;
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint64_t time_us;
    setupReceiver();
    set_frecuency_rx(CARRIER_FEQ + PIO_CENTER_OFFSET);
    set_frequency_deviation_rx(PIO_DEVIATION);
    set_datarate_rx(round(((double) PIO_BAUDRATE) * 0.985)); // experimental factor due to clock imprecision
    set_filter_bandwidth_rx(PIO_MIN_RX_BW);
    sleep_ms(1);
    RX_start_listen();
    printf("started listening\n");
    bool rx_ready = true;

    /* loop */
    while (true) {
        evt = get_event();
        switch(evt){
            case rx_assert_evt:
                // started receiving
                rx_ready = false;
                printf("rx_assert_evt...\n");
                //sleep_ms(10);
            break;
            case rx_deassert_evt:
                // finished receiving
                time_us = to_us_since_boot(get_absolute_time());
                status = readPacket(rx_buffer);
                printPacket(rx_buffer,status,time_us);
                RX_start_listen();
                rx_ready = true;
            break;
            case no_evt:
                // backscatter new packet if receiver is listening
                if (rx_ready){
                    /* generate new data */
                    generate_data(tx_payload_buffer, PAYLOADSIZE, true);

                    /* add header (10 byte) to packet */
                    add_header(&message[0], seq, header_tmplate);
                    /* add payload to packet */
                    memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

                    /* casting for 32-bit fifo */
                    for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
                        buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
                    }
                    /*put the data to FIFO*/
                    backscatter_send(pio,sm,buffer,sizeof(buffer));
                    //printf("Backscattered packet\n");

                    seq++;
                }
                sleep_ms(TX_DURATION);
            break;
        }
        sleep_us(100);
    }

    /* stop carrier and receiver - never reached */
    RX_stop_listen();
    stopCarrier();
}
