/**
 * Tobias Mages & Wenqing Yan
 *
 * Using the CC2500 to generate a 2450MHz unmodulated carrier with maximal output power of +1dBm.
 *
 * GPIO 17 (pin 22) Chip select
 * GPIO 18 (pin 24) SCK/spi0_sclk
 * GPIO 19 (pin 25) MOSI/spi0_tx
 * GPIO 21 GDO0: interrupt for received sync word
 *
 * The example uses SPI port 0.
 * The stdout has been directed to USB.
 *
 * The CC2500 can transmit and receive arbitrarily long packets. However, is FIFO is limited to 64 byte, 
 * out of which 4 byte are occupied by the length-field, sequence number and link quality information. 
 * This leaves 60 bytes for the payload.
 * To transmit larger payloads, it would be necessary to continoulsy empty the fifo while receiving a packet
 * which can lead to unwanted and timing dependent byte duplications as highlighted in the datasheet errata.
 * Therefore, we only start reading the fifo after the transmission is completed.
 *
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "receiver_CC2500.h"


void main() {
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

    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(RX_CSN, "SPI CS"));

    // Start receiver
    event_t evt = no_evt;
    Packet_status status;
    uint8_t buffer[RX_BUFFER_SIZE];
    setupReceiver();
    RX_start_listen();

    uint64_t time_us;
    
    while (true) {
        evt = get_event();
        switch(evt){
            case rx_assert_evt:
                // started receiving
            break;
            case rx_deassert_evt:
                // finished receiving
                time_us = to_us_since_boot(get_absolute_time());
                status = readPacket(buffer);
                printPacket(buffer,status,time_us);
                RX_start_listen();
            break;
            case no_evt:
            break;
        }
        sleep_us(10);
    }
    RX_stop_listen(); // never reached
}
