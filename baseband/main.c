/**
 * Tobias Mages & Wenqing Yan
 * Backscatter PIO
 * 02-March-2023
 */

#include <stdio.h>
#include "math.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "backscatter.pio.h"
#include "packet_generation.h"
#include "src/signal_helpers/signal_generate_helpers.h"
#include "chip_helpers.h"
//#include "chip_helpers/chip_helpers.h"

#define TX_DURATION 250 // send a packet every 250ms (when changing baud-rate, ensure that the TX delay is larger than the transmission time)
#define RECEIVER 1352 // define the receiver board either 2500 or 1352
#define PIN_TX1 6
#define PIN_TX2 27
#define PreambleSize 4
#define PayloadMaxSize 100

#define POLY 0x1201

int hello = SIGNAL_LEVEL_LOW;


int main() {
    stdio_init_all();
    PIO pio_1 = pio0;
    uint sm1 = 0;
    uint sm2 = 1;
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2, 0);

    gpio_init(3);
    gpio_set_dir(3, GPIO_OUT);
    gpio_put(3, 1);
    sleep_ms(1);
    uint offset1 = pio_add_program(pio_1, &backscatter_pio_0_program);

    // changing the clock frequency, 128 MHz to get the correct data rate for O-QPSK
#define SYS_FREQ_KHZ 64000
    if (set_sys_clock_khz(SYS_FREQ_KHZ, true)) {
        printf("clock correct");
    } else {
        printf("clock error");
    }
    printf("HELLO\n");

    backscatter_program_init(pio_1, sm1, offset1, PIN_TX1, PIN_TX2); // one antenna setup

//    while (true) {

//        uint8_t data[] = {0x00};
//        uint32_t chips[4 * 1];
//        uint32_t bufferlen = data_to_pio_input(data, 1, chips, 0);

//        uint32_t pio_data_buffer[signal_calc_len_for_signal_code(4 * 1, 1)];
//        int pio_data_buffer_len = convert_to_signal_code(
//                chips, bufferlen, 1, pio_data_buffer,
//                signal_calc_len_for_signal_code(4 * 1, 1));
    printf("MADE IT!");
        pio_sm_put_blocking(pio_1, 0, 0b01100011011111011110110111111111);
//        if (i == 0) {
    gpio_put(2, 1);
    gpio_put(3, 0);
//        }
//
//        for (uint32_t i = 0; i < pio_data_buffer_len; i++) {
//
//            //sleep_ms(1);
//        }

//        while (true) {
//
//        }
    /* put the data to FIFO */
//    backscatter_send(pio_1,pio_2,buffer,buffer_size(PAYLOADSIZE, HEADER_LEN));

//        seq++;
//    sleep_ms(TX_DURATION);
//    }
}
