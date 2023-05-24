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

#define TX_DURATION             50 // send a packet every 50ms
#define RECEIVER              1352 // define the receiver board either 2500 or 1352
#define PIN_TX1                  6
#define PIN_TX2                 27
#define CLOCK_DIV0              40 // larger
#define CLOCK_DIV1              36 // smaller
#define DESIRED_BAUD            89990
#define TWOANTENNAS             true

#define CARRIER_FEQ     2470000000
    /*Modified payloadsize*/
#define NEW_PAYLOAD 6
#define EXTRA_BYTE 2

int parity_calc(int bits)
{
    int r = 0;
    bool flag = false;
    while(!flag)
    {
        if((int)pow(2,r)>=bits+r+1)
        {
            flag=true;
            return r;
        }
       else
        r++;
    }
}


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

    sleep_ms(5000);

    /* setup backscatter state machine */
    PIO pio = pio0;
    uint sm = 0;
    struct backscatter_config backscatter_conf;
    uint16_t instructionBuffer[32] = {0}; // maximal instruction size: 32
    backscatter_program_init(pio, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS);

    //static uint8_t message[PAYLOADSIZE + HEADER_LEN];  // include 10 header bytes
    static uint8_t message[PAYLOADENC + HEADER_LEN];  // include 10 header bytes                                    // MODIFIED PAYLOADSIZE

    static uint8_t msg[PAYLOADSIZE + HEADER_LEN];

    //static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint32_t buffer[buffer_size(PAYLOADENC, HEADER_LEN)] = {0}; // initialize the buffer                   // MODIFIED PAYLOADSIZE
    //static uint32_t buffer[10] = {0};
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];                     // MODIFIED PAYLOADSIZE
    //uint8_t dummy_payload_buffer[10];
    uint8_t A[8]={0}; //array for the encoded decimal value
    //uint8_t tx_payload_buffer[PAYLOADSIZE];
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
                    /* generate new data */
                    generate_data(tx_payload_buffer, PAYLOADSIZE, true);   //MODIFIED PAYLOADSIZE


                    /* add header (10 byte) to packet */
                    add_header(&message[0], seq, header_tmplate);
                    /* add payload to packet */
                    //memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);


                    /*Adding Forward Error Correction encoding*/


                    memcpy(&msg[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);
                    printf("\n transaction buffer:\n");
                    for(int i = 0; i<PAYLOADSIZE; i++)
                    {
                        printf("tx_payload_buffer[%d]: %X\n",i,tx_payload_buffer[i]);
                    }
                    //uint16_t n = (uint16_t)msg[11]|((uint16_t)msg[10])<<8;
                    //uint32_t n = (uint32_t)msg[13]|((uint16_t)msg[12])<<8|((uint32_t)msg[11])<<16|((uint32_t)msg[10])<<24;
                    uint32_t n[2];
                    n[0] = (uint32_t)msg[15]|((uint16_t)msg[14])<<8|((uint32_t)msg[13])<<16|((uint32_t)0x00)<<24;       //splitting 6 byte into 3 bytes each, n1 & n2
                    n[1] = (uint32_t)msg[12]|((uint16_t)msg[11])<<8|((uint32_t)msg[10])<<16|((uint32_t)0x00)<<24;


                    int bin[32] = {0}; //to add the binary bits of the actual data
                    int buf[32] = {0};   // since one number n1 will be of size 32bit // to add the binary bits of n
                    int main_buf[64]={0};  //to add the binary bits of n1 and n2 after parity addition
                    int ctr,count,p_pos,p_bits,x,j_index,index;
                    int index_arr[32]={0}; //to store the position of data bits used to calculate the parity bit
                    uint32_t temp;
                    printf("start.....\n");
                    count =0;
                    while(count<2)  /*setting count to two because we are encoding 3 bytes each twice for 6 byte data*/
                    {
                        index = 0;
                        ctr =0;
                        p_pos= 0;
                        p_bits= 0;
                        j_index= 0;
                        x= 0;
                        temp =n[count];


                        //printf("temp:%X\n",temp);


                        while(temp>0){
                        bin[index] = temp%2;
                        temp=temp/2;
                        index++;
                        }

                         printf("Original binary data:\n");
                        for(int i =index-1; i>=0 ; i--)
                        {
                            printf("%d ",bin[i]);
                        }
                    
                    p_bits = parity_calc(index);  // calculates the number of parity bits
                    
                    
                    for(int i = (index+p_bits-1),j = index-1;i>=0;i--)
                    {
                        if(i!=0 && i!=1 && i!=3 && i!=7 && i!=15)  /*adding the data bits to positions other than the parity positions*/
                        {
                            buf[i] = bin[j];
                            j--;
                        }
                    }
                    printf("\nparity added buffer:\n");
                    for(int i=index+p_bits-1;i>=0;i--)
                    {
                        printf("%d ",buf[i]);
                    }


                    //to get the index of the parity bit and its corresponding data bits
                        while(x<p_bits)
                        {
                            p_pos = (int)pow(2,x)-1;
                            ctr = (int)pow(2,x);
                            printf("p_pos: %d\n",p_pos);
                            for(int i=p_pos,j=0;i<(index+p_bits);)
                            {
                                if(ctr!=0)
                                {
                                    index_arr[j] = i;     /*storing the location of parity bits and their corresponding data bits*/
                                    j++;
                                    ctr--;
                                    if(ctr!=0)
                                    i++;                
                                }
                                else{
                                    ctr = (int)pow(2,x);
                                    i = ctr + i + 1;
                                }
                                //printf("%d:%d , ",index_arr[j],j);
                                j_index = j;                                
                            }
                            //to add parity values to its position
                            for(int i =1 ; i<j_index;i++)
                            {
                                buf[p_pos] = buf[p_pos] ^ buf[index_arr[i]];
                            }                              
                            x++;
                        }
                        for(int i =0;i<32;i++)
                        {
                            if(count == 0)
                            main_buf[i] = buf[i];
                            else
                            main_buf[i+32] = buf[i];
                        }
                        
                        for (int i=63;i>=0;i--)
                        {
                            printf("%d ", main_buf[i]);
                        }
                         printf("\n");
                        count++;
                    }
                        printf("\n");
                        // for (int i=63;i>=0;i--)
                        // {
                        //     printf("%d ", main_buf[i]);
                        // }
                        int c = 0;
                        int k =0;
                    
                        //binary to decimal conversion
                        while(k<8)
                        {
                            for(int i=c,j=0;i<c+8;i++,j++)
                            {
                                A[k] = A[k]+((int)pow(2,j)*main_buf[i]);
                            }
                            c=c+8;
                            k++;
                        }
                        printf("\n");
                        for(int i=7; i>=0; i--)
                        {
                            printf("%X ",A[i]); 
                        }




                    memcpy(&message[HEADER_LEN], A, PAYLOADENC);  //MODIFIED PAYLOADSIZE
                    printf("\nmessage:\n");
                    for (int i = 0 ; i<HEADER_LEN+PAYLOADENC;i++)
                    {
                        printf("%X ",message[i]);
                    }
                    printf("\n****************\n");
                    /* casting for 32-bit fifo */
                    //for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) 
                    for (uint8_t i=0; i < buffer_size(PAYLOADENC, HEADER_LEN); i++) {
                    
                        buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
                        printf("%X ",buffer[i]);
                    }
                    printf("\n****************\n");
                    /*put the data to FIFO*/
                    backscatter_send(pio,sm,buffer,sizeof(buffer));
                    //printf("Backscattered packet\n");
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
