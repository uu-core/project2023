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
#include "backscatter.h"
#include "carrier_CC2500.h"
#include "receiver_CC2500.h"
#include "packet_generation.h"


#define RADIO_SPI             spi0
#define RADIO_MISO              16
#define RADIO_MOSI              19
#define RADIO_SCK               18

#define TX_DURATION            50 // send a packet every 250ms (when changing baud-rate, ensure that the TX delay is larger than the transmission time)
#define RECEIVER              1352 // define the receiver board either 2500 or 1352
#define PIN_TX1                  6
#define PIN_TX2                 27
#define PIN_TX3                  9
#define PIN_TX4                 22
#define CLOCK_DIV0              24 // larger
#define CLOCK_DIV1              22 // smaller
#define DESIRED_BAUD        50000
#define TWOANTENNAS          true
#define SYNCPIN                 10

#define CARRIER_FEQ     2450000000

#define HEADER_LEN ((RECEIVER == 13521)? HEADER_LEN_LRM: HEADER_LEN_UNCODED)

int main() {
    /* setup SPI */
    uint16_t phs_conf = 0;
    uint16_t rxsel    = RECEIVER;

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

    gpio_init(SYNCPIN);
    gpio_set_dir(SYNCPIN, GPIO_OUT);
    gpio_put(SYNCPIN, 0);

    sleep_ms(15000);

    /* setup backscatter state machine */
    uint sm = 0;
    struct backscatter_config backscatter_conf;
    struct backscatter_config backscatter_conf2;
    uint16_t instructionBuffer[32] = {0}; // maximal instruction size: 32
    uint16_t instructionBuffer2[32] = {0}; // maximal instruction size: 32
    backscatter_program_init(pio0, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
    backscatter_program_init(pio1, sm, PIN_TX3, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 0);

    static uint8_t message[buffer_size2(PAYLOADSIZE+2, HEADER_LEN, RECEIVER, DSSS)*4] = {0};  // include 10 header bytes
    static uint32_t buffer[buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS)]   = {0}; // initialize the buffer

    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];
    uint8_t tx_encoded_payload_buffer[2*DSSS*PAYLOADSIZE];

    /* Setup carrier */
    setupCarrier();
    set_frecuency_tx(CARRIER_FEQ);
    sleep_ms(1);

    /* Start Receiver */
    event_t evt = no_evt;
    Packet_status status;
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint64_t time_us;
    setupReceiver();
    set_frecuency_rx(CARRIER_FEQ + backscatter_conf.center_offset);
    set_frequency_deviation_rx(backscatter_conf.deviation);
    set_datarate_rx(backscatter_conf.baudrate);
    set_filter_bandwidth_rx(backscatter_conf.minRxBw);
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
                    
                    /* add header (10 byte) to packet */
                    add_header(&message[0], seq, header_tmplate, HEADER_LEN);

                    /* generate new data */
                    generate_data(tx_payload_buffer, PAYLOADSIZE, true);
                    // memset(tx_payload_buffer, 0xa, PAYLOADSIZE);
                    if (rxsel == 13521)
                    {
                        uint8_t whiten_out[PAYLOADSIZE] = {0};
                        whiten_data(tx_payload_buffer, whiten_out, PAYLOADSIZE);
                        encode_payload(whiten_out, tx_encoded_payload_buffer, 2*DSSS*PAYLOADSIZE);
                        /* add payload to packet */
                        memcpy(&message[HEADER_LEN], tx_encoded_payload_buffer, 2*DSSS*PAYLOADSIZE);
                    } else
                    {
                        /* add payload to packet */
                        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);
                    }
                    
                    /* casting for 32-bit fifo */
                    for (uint8_t i=0; i < buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS); i++) {
                        buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
                        printf("%08x ", buffer[i]);
                    }
                    printf("\n");
                    /* put the data to FIFO (start backscattering) */
                    startCarrier();
                    sleep_ms(1); // wait for carrier to start
                    if (buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS) > 6)
                    {
                        int temp_s = buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS);
                        int buf_i  = 0;
                        while (temp_s > 6)
                        {
                            backscatter_send(pio0,sm,&buffer[buf_i*6],6);
                            backscatter_send(pio1,sm,&buffer[buf_i*6],6);
                            gpio_put(SYNCPIN, 1);
                            temp_s -= 6;
                            buf_i++;
                        }
                        backscatter_send(pio0,sm,&buffer[buf_i*6],temp_s);
                        backscatter_send(pio1,sm,&buffer[buf_i*6],temp_s);
                        
                    } else
                    {
                        backscatter_send(pio0,sm,buffer,buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS));
                        backscatter_send(pio1,sm,buffer,buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS));
                        gpio_put(SYNCPIN, 1);
                    }
                    
                    
                    
                    sleep_ms(ceil((((double) buffer_size2(PAYLOADSIZE, HEADER_LEN, RECEIVER, DSSS))*8000.0)/((double) DESIRED_BAUD))+3); // wait transmission duration (+3ms)
                    stopCarrier();
                    gpio_put(SYNCPIN, 0);
                    /* increase seq number*/ 
                    seq++;
                }
                sleep_ms(TX_DURATION);
            break;
            case tag_reset_evt:
                switch (phs_conf)
                {
                case 0:
                    backscatter_program_init(pio0, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX3, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 60);
                    printf("6-27-zero-9-22-60\n");
                    phs_conf++;
                    break;
                    
                case 1:
                    backscatter_program_init(pio0, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX3, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 90);
                    printf("6-27-zero-9-22-90\n");
                    phs_conf++;
                    break;
                    
                case 2:
                    backscatter_program_init(pio0, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX3, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 120);
                    printf("6-27-zero-9-22-120\n");
                    phs_conf++;
                    break;
                    
                case 3:
                    backscatter_program_init(pio0, sm, PIN_TX1, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX3, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 90);
                    printf("6-22-zero-9-27-90\n");
                    phs_conf++;
                    break;
                
                case 4:
                    backscatter_program_init(pio0, sm, PIN_TX3, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX1, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 90);
                    printf("9-27-zero-6-22-90\n");
                    phs_conf++;
                    break;
                
                case 5:
                    backscatter_program_init(pio0, sm, PIN_TX3, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX1, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 180);
                    printf("9-27-zero-6-22-180\n");
                    phs_conf++;
                    break;
                
                default:
                    backscatter_program_init(pio0, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS, 0);
                    backscatter_program_init(pio1, sm, PIN_TX3, PIN_TX4, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf2, instructionBuffer2, TWOANTENNAS, 0);
                    printf("default\n");
                    phs_conf = 0;
                    break;
                }
            break;
        }
        sleep_ms(1);
    }

    /* stop carrier and receiver - never reached */
    RX_stop_listen();
    stopCarrier();
}
