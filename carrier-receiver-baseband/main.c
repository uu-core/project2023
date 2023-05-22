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

#define RADIO_SPI spi0
#define RADIO_MISO 16
#define RADIO_MOSI 19
#define RADIO_SCK 18

#define TX_DURATION 50 // send a packet every 50ms
#define RECEIVER 2500  // define the receiver board either 2500 or 1352
#define PIN_TX1 6
#define PIN_TX2 27
#define CLOCK_DIV0 20 // larger
#define CLOCK_DIV1 18 // smaller
#define DESIRED_BAUD 100000
#define TWOANTENNAS true

#define CARRIER_FEQ 2450000000

// hamming part

const uint8_t rows = 4;
const uint8_t cols = 8;

void print(uint32_t data_snd)
{
    for (uint8_t r = 0; r < rows; r++)
    {
        for (uint8_t c = 0; c < cols; c++)
        {
            printf("%d ", (data_snd & (1 << ((cols * r) + c))) >> ((cols * r) + c));
        }
        printf("\n");
    }
    printf("\n");
}

uint8_t calc_parity_col(uint8_t block, uint32_t data)
{
    uint8_t parity = 0;
    uint8_t block_indx = 0;
    for (uint8_t r = 0; r < rows; r++)
    {
        for (uint8_t c = block; c < cols; c++)
        {
            if ((uint8_t)(c / block) % 2 == 0)
                continue;
            parity = parity ^ ((data & (1 << ((cols * r) + c))) >> ((cols * r) + c));
        }
    }
    return parity;
}

uint8_t calc_parity_row(uint8_t block, uint32_t data)
{
    uint8_t parity = 0;
    uint8_t count = 0;
    for (uint8_t r = block; r < rows; r++)
    {
        if ((uint8_t)(r / block) % 2 == 0)
            continue;
        for (uint8_t c = 0; c < cols; c++)
        {
            parity = parity ^ ((data & (1 << ((cols * r) + c))) >> ((cols * r) + c));
        }
    }
    return parity;
}

// The first 6 bits should not be set
uint32_t hamming_encode(uint32_t data)
{
    const uint8_t indx_data[26] = {3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    const uint8_t indx_parity[6] = {0, 1, 2, 4, 8, 16}; // 0 is not a parity bit but is used for hamming extended
    uint32_t data_snd = 0;
    uint32_t bit_position = 0;
    uint32_t data_bit_value = 0;
    uint32_t data_at_position = 0;

    /* Start with the lowest bit, i.e. bit 6 in the 32-bit integer */
    /* Insert the smallest number in the first position of the message */
    for (int i = 0; i < 24; i++)
    {
        bit_position = (1 << indx_data[i]);         // 2^indx_data[i]
        data_bit_value = data & (1 << i);           // 2^i or 0
        data_at_position = ((data_bit_value) >> i); // 0 or 1
        data_snd += bit_position * data_at_position;
    }

    // Calculate the parity bit and insert it to the data_snd
    data_snd += (1 << indx_parity[1]) * calc_parity_col(1, data_snd);
    data_snd += (1 << indx_parity[2]) * calc_parity_col(2, data_snd);
    data_snd += (1 << indx_parity[3]) * calc_parity_col(4, data_snd);
    data_snd += (1 << indx_parity[4]) * calc_parity_row(1, data_snd);
    data_snd += (1 << indx_parity[5]) * calc_parity_row(2, data_snd);

    return data_snd;
}

uint32_t encode_3_uint8(uint8_t data8_1, uint8_t data8_2, uint8_t data8_3)
{
    uint8_t data8_dummy = 0b00000000;
    uint32_t to_encode = (uint32_t)(data8_dummy << 24) |
                         (uint32_t)(data8_1 << 16) |
                         (uint32_t)(data8_2 << 8) |
                         (uint32_t)(data8_3 << 0);

    // printf("Before   : %x %x %x %x\n", data8_1, data8_2, data8_3, data8_dummy);
    // printf("To encode: %x\n", to_encode);
    uint32_t encoded = hamming_encode(to_encode);
    // printf("Encoded: %x\n", encoded);
    return encoded;
}

// end of hamming part

int main()
{
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

    sleep_ms(5000);

    /* setup backscatter state machine */
    PIO pio = pio0;
    uint sm = 0;
    struct backscatter_config backscatter_conf;
    uint16_t instructionBuffer[32] = {0}; // maximal instruction size: 32
    backscatter_program_init(pio, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS);

    static uint8_t message[PAYLOADSIZE_ENC + HEADER_LEN];                   // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE_ENC, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];
    uint8_t tx_enc_payload_buffer[PAYLOADSIZE_ENC];

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
    set_frecuency_rx(CARRIER_FEQ + backscatter_conf.center_offset);
    set_frequency_deviation_rx(backscatter_conf.deviation);
    set_datarate_rx(backscatter_conf.baudrate);
    set_filter_bandwidth_rx(backscatter_conf.minRxBw);
    sleep_ms(1);
    RX_start_listen();
    printf("started listening\n");
    bool rx_ready = true;

    /* loop */
    while (true)
    {
        evt = get_event();
        switch (evt)
        {
        case rx_assert_evt:
            // started receiving
            rx_ready = false;
            break;
        case rx_deassert_evt:
            // finished receiving
            time_us = to_us_since_boot(get_absolute_time());
            status = readPacket(rx_buffer);
            printPacket(rx_buffer, status, time_us);
            RX_start_listen();
            rx_ready = true;
            break;
        case no_evt:
            // backscatter new packet if receiver is listening
            if (rx_ready)
            {
                /* generate new data */
                generate_data(tx_payload_buffer, PAYLOADSIZE, true);

                /* add header (10 byte) to packet */
                add_header(&message[0], seq, header_tmplate);
                /* add payload to packet */
                memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

                /* casting for 32-bit fifo */
                for (uint8_t i = 0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++)
                {
                    buffer[i] = ((uint32_t)message[4 * i + 3]) | (((uint32_t)message[4 * i + 2]) << 8) | (((uint32_t)message[4 * i + 1]) << 16) | (((uint32_t)message[4 * i]) << 24);
                }

                /*put the data to FIFO*/
                backscatter_send(pio, sm, buffer, sizeof(buffer));
                // printf("Backscattered packet\n");
                seq++;
            }
            sleep_ms(TX_DURATION);
            break;
        }
        sleep_ms(1);
    }

    /* stop carrier and receiver - never reached */
    RX_stop_listen();
    stopCarrier();
}
