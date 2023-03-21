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
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "receiver_CC2500.h"
#include "carrier_CC2500.h"

queue_t event_queue;

// Address Config = No address check
// Base Frequency = 2456.596924
// CRC Autoflush = false
// CRC Enable = true
// Carrier Frequency = 2456.596924
// Channel Number = 0
// Channel Spacing = 405.456543
// Data Format = Normal mode
// Data Rate = 98.587
// Deviation = 355.468750
// Device Address = 0
// Manchester Enable = false
// Modulated = true
// Modulation Format = 2-FSK
// Packet Length = 255
// Packet Length Mode = Variable packet length mode. Packet length configured by the first byte after sync word
// Preamble Count = 4
// RX Filter BW = 812.500000
// Sync Word Qualifier Mode = 30/32 sync word bits detected
// TX Power = 0
// Whitening = false

RF_setting cc2500_receiver[20] = {
  {.address = 0x02, .value = 0x06}, // CC2500_IOCFG0: GDO0Output Pin Configuration
  /* GDO0 config: -> used to generate interrupts when sync word is found
   * Asserts when sync word has been sent / received, and de-asserts at the end of the packet.
   * In RX, the pin will de-assert when the optional address check fails or the RX FIFO overflows.
   */
  {.address = 0x08, .value = 0x05}, // CC2500_PKTCTRL0: Packet Automation Control
  {.address = 0x0b, .value = 0x0A}, // CC2500_FSCTRL1: Frequency Synthesizer Control
  {.address = 0x0e, .value = 0x7C}, // CC2500_FREQ1: Frequency Control Word, Middle Byte
  {.address = 0x0f, .value = 0x08}, // CC2500_FREQ0: Frequency Control Word, Low Byte
  {.address = 0x10, .value = 0x0B}, // CC2500_MDMCFG4: Modem Configuration
  {.address = 0x11, .value = 0xF1}, // CC2500_MDMCFG3: Modem Configuration
  {.address = 0x12, .value = 0x03}, // CC2500_MDMCFG2: Modem Configuration
  {.address = 0x13, .value = 0x23}, // CC2500_MDMCFG1: Modem Configuration
  {.address = 0x14, .value = 0xFF}, // CC2500_MDMCFG0: Modem Configuration
  {.address = 0x15, .value = 0x76}, // CC2500_DEVIATN: Modem Deviation Setting
  {.address = 0x18, .value = 0x18}, // CC2500_MCSM0: Main Radio Control State Machine Configuration
  {.address = 0x19, .value = 0x1D}, // CC2500_FOCCFG: Frequency Offset Compensation Configuration
  {.address = 0x1a, .value = 0x1C}, // CC2500_BSCFG: Bit Synchronization Configuration
  {.address = 0x1b, .value = 0xC7}, // CC2500_AGCCTRL2: AGC Control
  {.address = 0x1c, .value = 0x00}, // CC2500_AGCCTRL1: AGC Control
  {.address = 0x1d, .value = 0xB0}, // CC2500_AGCCTRL0: AGC Control
  {.address = 0x21, .value = 0xB6}, // CC2500_FREND1: Front End RX Configuration
  {.address = 0x25, .value = 0x00}, // CC2500_FSCAL1: Frequency Synthesizer Calibration
  {.address = 0x26, .value = 0x11}, // CC2500_FSCAL0: Frequency Synthesizer Calibration
};

void cs_select_rx() {
    asm volatile("nop \n nop \n nop");
    gpio_put(RX_CSN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

void cs_deselect_rx() {
    asm volatile("nop \n nop \n nop");
    gpio_put(RX_CSN, 1);
    asm volatile("nop \n nop \n nop");
}

void write_strobe_rx(uint8_t cmd) {
    cs_select_rx();
    spi_write_blocking(RADIO_SPI, &cmd, 1);
    cs_deselect_rx();
    sleep_ms(1);
}

void write_register_rx(RF_setting set) {
    uint8_t buf[2];
    buf[0] = set.address;
    buf[1] = set.value;
    cs_select_rx();
    spi_write_blocking(RADIO_SPI, buf, 2);
    cs_deselect_rx();
    sleep_ms(1);
}

void write_registers_rx(RF_setting* sets, uint8_t len) {
    uint8_t buf[2];
    cs_select_rx();
    for (int i = 0; i < len; i++) {
        buf[0] = sets[i].address;
        buf[1] = sets[i].value;
        spi_write_blocking(RADIO_SPI, buf, 2);
    }
    cs_deselect_rx();
}

/* ISR */
void receiver_isr(uint gpio, uint32_t events)
{
    event_t evt;
    switch(gpio){
        case RX_GDO0_PIN:
            switch(events){
                case GPIO_IRQ_EDGE_RISE:
                    evt = rx_assert_evt;
                    queue_try_add(&event_queue, &evt);
                    break;
                case GPIO_IRQ_EDGE_FALL:
                    evt = rx_deassert_evt;
                    queue_try_add(&event_queue, &evt);
                    break;
            }
        break;
    }
}

void setupReceiver(){
    write_strobe_rx(SIDLE); // ensure IDLE mode with command strobe: SIDLE
    write_registers_rx(cc2500_receiver,20);

    /* Event queue setup */
    queue_init(&event_queue, sizeof(event_t), EVENT_QUEUE_LENGTH);

    /* Reset the queue */
    while(queue_try_remove(&event_queue, NULL));

    /* GDO0 setup as interrupt */
    gpio_set_irq_enabled_with_callback(RX_GDO0_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &receiver_isr);

}

// continously listen for packets
void RX_start_listen(){
    write_strobe_rx(SIDLE);
    RF_setting set = {.address = 0x17, .value = 0x00};    // after receiving a packet, return to idle
    //RF_setting set = {.address = 0x17, .value = 0x0C}; // after receiving a packet, listen for next one
    write_register_rx(set);
    write_strobe_rx(SFRX); // clear FIFO
    write_strobe_rx(SRX);  // start listening (enter RX mode with command strobe: SRX)
}

// stop listening
void RX_stop_listen(){
    write_strobe_rx(SIDLE); // stop listening (enter IDLE mode with command strobe: SIDLE)
}

Packet_status readPacket(uint8_t *buffer){
    Packet_status status;
    uint8_t tmp_buffer[2];
    // since the provided length of a packet might be corrupted, read length from fifo status
    cs_select_rx();
    spi_read_blocking(RADIO_SPI, 0xFB, tmp_buffer, 2);               // read RX FIFO status
    cs_deselect_rx();
    status.overflowed = (bool) (tmp_buffer[1] & 0x80);
    if (!status.overflowed){
        status.len = (tmp_buffer[1] & 0x7F) - 2;
        cs_select_rx();
        spi_read_blocking(RADIO_SPI, 0xFF, tmp_buffer, 1);               // sart burst access to RX FIFO
        spi_read_blocking(RADIO_SPI, 0xFF, buffer, min(min(status.len, 62), RX_BUFFER_SIZE));  // start reading from burst (max. 62 bytes of packet)
        spi_read_blocking(RADIO_SPI, 0xFF, tmp_buffer,  2);              // read quality information
        cs_deselect_rx();
        status.CRCcheck = (bool) (tmp_buffer[1] & 0x80);
        status.LinkQualityIndicator = (tmp_buffer[1] & 0x7F);
        if(tmp_buffer[0] >= 128){
            status.RSSI = (((int32_t) tmp_buffer[0]) - 256)/2 - 70;
        }else{
            status.RSSI = ((int32_t) tmp_buffer[0])/2 - 70;
        }
    }
    return status;
}

void printPacket(uint8_t *packet, Packet_status status, uint64_t time_us){
    // generate timestamp since boot-up
    uint64_t time_rem;
    uint32_t hours    = (int32_t) (time_us  / ((uint64_t) 36 * (uint64_t) 100000000));
    time_rem          =           (time_us  % ((uint64_t) 36 * (uint64_t) 100000000));
    uint32_t minutes  = (int32_t) (time_rem / (60 *   1000000));
    time_rem          =           (time_rem % (60 *   1000000));
    uint32_t  sec     = (int32_t) (time_rem / (1000000));
    time_rem          =           (time_rem % (1000000));
    uint32_t msec     = (int32_t) (time_rem / (1000));
    printf("%02d:%02d:%02d.%03d | ", hours, minutes, sec, msec);
    if(status.overflowed){
        printf("packet overflow (possible length field corrupted) | CRC error\n");
    }else{
        for(uint8_t i = 0; i < min(status.len,RX_BUFFER_SIZE); i++){
            printf("%02x ", packet[i]);
        }
        printf("| ");
        printf("%d ", status.RSSI);
        if(status.CRCcheck){
            printf("CRC pass\n");
        }else{
            printf("CRC error\n");
        }
    }
}

event_t get_event(void)
{
    event_t evt = no_evt;
    if (queue_try_remove(&event_queue, &evt))
    {
        return evt;
    }
    return no_evt;
}
