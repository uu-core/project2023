/**
 * Tobias Mages & Wenqing Yan
 * Backscatter PIO
 * 02-March-2023
 */

#include <cstdio>
#include <cmath>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "backscatter.pio.h"
#include "src/pio_bytecode_generator.h"
//#include "chip_helpers/chip_helpers.h"


#define PIN_TX 6
#define PIN_PIO_TRIGGER 3
#define PIN_OSC_INV_TRIGGER 2


void set_up_gpio_triggers() {
    gpio_init(PIN_OSC_INV_TRIGGER);
    gpio_set_dir(PIN_OSC_INV_TRIGGER, GPIO_OUT);
    gpio_put(PIN_OSC_INV_TRIGGER, false);

    gpio_init(PIN_PIO_TRIGGER);
    gpio_set_dir(PIN_PIO_TRIGGER, GPIO_OUT);
    gpio_put(PIN_PIO_TRIGGER, true);
}

void set_up_pio(PIO pio) {
    uint prgm_offset = pio_add_program(pio, &backscatter_pio_0_program);
    backscatter_program_init(pio, 0, prgm_offset, PIN_TX);
}

void set_clock_frequency() {
#define SYS_CLK_FREQ_KHZ 128000
    if (set_sys_clock_khz(SYS_CLK_FREQ_KHZ, true)) {
        printf("clock correct\n");
    } else {
        printf("clock error");
    }
}


int main() {
    // enable logging via usb/uart
    stdio_init_all();
    // set up high/low triggers to start pio program
    set_up_gpio_triggers();

    // changing the clock frequency, 128 MHz to get the correct data rate for O-QPSK
    set_clock_frequency();

    sleep_ms(1000);

    PIO pio = pio0;
    set_up_pio(pio);
    sleep_ms(5000);
    printf("Starting generating data\n");
    // Valid IEEE 802.15.4 Packet with payload "01 02 03 04 05 06 07 08 09 0A" is 00000000A71741880B222234124444CDAB0102030405060708090A4B49

    std::vector<uint32_t> data = pio_bytecode::generate("00000000A71741880B222234124444CDAB0102030405060708090A4B49");

    printf("Starting sending data\n");

    gpio_put(3, true);
    int idx = 0;
    while (true) {

        for (unsigned long i: data) {
            pio_sm_put_blocking(pio, 0, i);
            if (idx == 0) {
                gpio_put(2, true);
                gpio_put(3, false);
            }
        }
        sleep_ms(500);

        if (idx % 5) {
            sleep_ms(1000);
        }
        idx++;


    }

}
