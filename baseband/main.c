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
//#include "chip_helpers/chip_helpers.h"

#define TX_DURATION 250 // send a packet every 250ms (when changing baud-rate, ensure that the TX delay is larger than the transmission time)
#define RECEIVER 1352 // define the receiver board either 2500 or 1352
#define PIN_TX1 6
#define PIN_TX2 27
#define PreambleSize 4
#define PayloadMaxSize 100

#define POLY 0x8408
// Define IEEE 802.15.4 frame structure
typedef struct {
    unsigned short FCP;
    unsigned char SN;
    unsigned short destination_PAN;
    unsigned short destination_address;
    unsigned short source_PAN;
    unsigned short source_address;
    uint16_t FCS;
    uint8_t Preamble[PreambleSize];
    uint8_t SFD;
    uint8_t len;
    uint8_t payload[PayloadMaxSize];
} Frame;

#define MHR_SIZE 10

void generateMHR(Frame frame, unsigned char output[]) {
    output[0] = frame.FCP & 0xFF;
    output[1] = (frame.FCP >> 8) & 0xFF;
    output[2] = frame.SN;
    output[3] = frame.destination_PAN & 0xFF;
    output[4] = (frame.destination_PAN >> 8) & 0xFF; 
    output[5] = frame.destination_address & 0xFF;
    output[6] = (frame.destination_address >> 8) & 0xFF;
    output[7] = frame.source_PAN & 0xFF;
    output[8] = (frame.source_PAN >> 8) & 0xFF;
    output[9] =  frame.source_address & 0xFF;
    output[10] = (frame.source_address >> 8) & 0xFF;
}

uint16_t crc16(uint8_t *data_p, uint16_t length) {
    uint8_t i;
    uint16_t data;
    uint16_t crc = 0xffff;

    if (length == 0)
        return (~crc);

    do {
        for (i = 0, data = (uint16_t)0xff & *data_p++; i < 8; i++, data >>= 1) {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else
                crc >>= 1;
        }
    } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return crc;
}

int main() {
    PIO pio_1 = pio0;
    PIO pio_2 = pio1;
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
    uint offset2 = pio_add_program(pio_2, &backscatter_pio_1_program);


    //backscatter_program_init(pio1, sm, offset, PIN_TX1, PIN_TX2); // two antenna setup
    backscatter_program_init(pio_1, sm1, offset1, PIN_TX1,PIN_TX2); // one antenna setup
    backscatter_program_init(pio_2, sm2, offset2, PIN_TX1,PIN_TX2); // one antenna setup

    static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];


    //uint32_t TEST_VALUE = 0b12345678901234567890123456789012';
    uint32_t TEST_VALUE =   0b10110100101101001011010010110100;

    Frame frame;
    #define PAYLOAD_SIZE 8
    uint8_t payload[PAYLOAD_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    unsigned char MHR[MHR_SIZE];

    // Generate a Frame instance
    frame.FCP = 0x8841;
    frame.SN = 0x01;
    frame.destination_PAN = 0x2222;
    frame.destination_address = 0x1234;
    frame.source_PAN = 0x4444;
    frame.source_address = 0xABCD;
    memcpy(frame.payload, payload, PAYLOAD_SIZE);

    generateMHR(frame, MHR);
    
    printf("MHR: ");
    for (int i = 0; i < MHR_SIZE; i++) {
        printf("%02X ", MHR[i]);
    }
    printf("\n");

    frame.FCS = crc16(payload, PAYLOAD_SIZE);
    
    printf("FCS of data is: 0x%04x\n", frame.FCS);


    uint8_t MPDU[MHR_SIZE + 2]; // Additional 2 bytes for the CRC value

    // Combine MHR and CRC value (FCS)
    for (int i = 0; i < MHR_SIZE; i++) {
        MPDU[i] = MHR[i];
    }

    // Convert CRC value to bytes and append to MPDU
    MPDU[MHR_SIZE] = frame.FCS >> 8; // High byte
    MPDU[MHR_SIZE + 1] = frame.FCS & 0xFF; // Low byte
    
     // Print MPDU
    printf("MPDU: ");
    for (int i = 0; i < MHR_SIZE + 2; i++) {
        printf("%02X ", MPDU[i]);
    }
    printf("\n");
    //Assign Preamble , SFD and length
     for (uint8_t i = 0 ; i < sizeof(PreambleSize);i++)
    {
        frame.Preamble[i] = 0x00;
    }
    frame.SFD = 0xA7;
    frame.len = MHR_SIZE + 2 + PAYLOAD_SIZE;
    
    uint8_t PPDU[sizeof(PreambleSize) + sizeof(frame.SFD) + sizeof(frame.len)];

    memcpy(PPDU, frame.Preamble, sizeof(PreambleSize));

    PPDU[sizeof(PreambleSize)] = frame.SFD;

    PPDU[sizeof(frame.Preamble) + sizeof(frame.SFD)] = frame.len;

    // Copy MPDU to PPDU array after len
    memcpy(PPDU[sizeof(frame.Preamble) + sizeof(frame.SFD) + sizeof(frame.len)], MPDU, MHR_SIZE + 2);

    // Print PPDU array
    printf("PPDU array: ");
    for (int i = 0; i < sizeof(PPDU); i++) {
        printf("%02X ", PPDU[i]);
    }
    printf("\n");
    printf("Size of PPDU %d\n", sizeof(PPDU));

    uint len_inputBuffer = sizeof(frame.Preamble) + sizeof(frame.SFD) + sizeof(frame.len) + MHR_SIZE + sizeof(frame.FCS) + PAYLOAD_SIZE;
    while (true) {

//        /* generate new data */
//        generate_data(tx_payload_buffer, PAYLOADSIZE, true);
//
//        /* add header (10 byte) to packet */
//        add_header(&message[0], seq, header_tmplate);
//        /* add payload to packet */
//        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);
//
//        /* casting for 32-bit fifo */
//        for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
//            buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
//        }
        uint32_t outbuffer[4*len_inputBuffer];
        uint bufferlen = data_to_pio_input(PPDU,len_inputBuffer,outbuffer,0);
        for(uint32_t i = 0; i < bufferlen; i++){
            pio_sm_put_blocking(pio_1, 0, outbuffer[i]);
            pio_sm_put_blocking(pio_2, 1, outbuffer[i]);
            if(i==0)
            {
                gpio_put(2, 1);
                gpio_put(3, 0);
            }
                    //sleep_ms(1);
        }

        /* put the data to FIFO */
        //backscatter_send(pio_1,pio_2,buffer,buffer_size(PAYLOADSIZE, HEADER_LEN));

        seq++;
        sleep_ms(TX_DURATION);
    }
}
