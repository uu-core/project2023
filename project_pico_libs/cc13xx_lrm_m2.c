#include "cc13xx_lrm_m2.h"


bool convshift[K] = {0};
bool pn9_shift[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

void conv_encoder(bool in, bool* a0, bool* a1){
    bool temp[K] = {0};
    // printf("cf i %d\n", in);
    for (int i = 0; i < K; i++)
    {
        temp[i] = convshift[i];
    }
    for (int i = 0; i < K-1; i++)
    {
        convshift[i+1] = temp[i];
    }
    convshift[0] = in;
    *a0 = (((temp[0]^temp[3])^temp[4])^temp[5])^temp[6];
    *a1 = (((temp[0]^temp[1])^temp[3])^temp[4])^temp[6];
    
}

void reset_conv_reg(){
    memset(convshift, 0, K);
}

void dsss(bool in, char mode, bool* out){
    switch (mode)
    {
    case 1:
        printf("Unsupported mode.");
        break;
    case 2:
        if (in==0)
        {
            out[0] = 0;
            out[1] = 0;
        } else {
            out[0] = 1;
            out[1] = 1;
        }
        break;
    case 4:
        if (in==0)
        {
            out[0] = 1;
            out[1] = 1;
            out[2] = 0;
            out[3] = 0;
        } else {
            out[0] = 0;
            out[1] = 0;
            out[2] = 1;
            out[3] = 1;
        }
        break;
    case 8:
        printf("Unsupported mode.");
        break;
    
    default:
        printf("Unsupported mode.");
        break;
    }
}


void encode_payload(uint8_t* in, uint8_t* out, int payloadsize){

    bool a0, a1;
    bool x[16];
    int  k, n;

    reset_conv_reg();
    n = 0;
    
    for (int i = 0; i < payloadsize; i++)
    {
        k = 0;
        // printf("\nocnv in %d\n", in[i]);
        for (int j = 0; j < 8; j++)
        {
            // printf("\nocnv in %02x, %02x\n", in[i], (in[i]&(0x80>>j)));
            ((in[i]&(0x80)>>j)>0)? conv_encoder(1, &a0, &a1) : conv_encoder(0, &a0, &a1);
            // printf("%d%d \n", a0, a1);
            // printf("cons %02x \n", (uint8_t) (convshift[6]<<6 | convshift[5]<<5 | convshift[4]<<4 | convshift[3]<<3 | convshift[2]<<2 | convshift[1]<<1 | convshift[0]));
            dsss(a0, DSSS, x+k);
            dsss(a1, DSSS, x+k+DSSS);
            if ((k+2*DSSS)%8 == 0)
            {
                out[n] = 0;
                for (int p = 0; p < 8; p++)
                {
                    out[n] = out[n] |  (x[p] << (7-p));
                }
                // printf("outn %02x \n", out[n]);
                n++;
                k = 0;
            }else
            {
                k += 2*DSSS;
            }
            
        }
        
    }
    
}


void generate_pn9_chip(bool restart){
    bool temp[9] = {0};
    if (restart)
    {
        memset(pn9_shift, 1, 9);
    } else{
        for (int i = 0; i < 9; i++)
        {
            temp[i] = pn9_shift[i];
        }
        for (int i = 1; i < 9; i++)
        {
            pn9_shift[i-1] = temp[i];
        }
        pn9_shift[8] = temp[5] ^ temp[0];
    }
    return;
}

void whiten_data(uint8_t* white_i, uint8_t* white_o, uint16_t payload_size){
    
    for (int j = 0; j < payload_size; j++)
    {
        if (j==0)
        {
            generate_pn9_chip(true);
        }else
        {
            for (int k = 0; k < 8; k++)
            {
                generate_pn9_chip(false);
            }
        }
        white_o[j] = white_i[j] ^ ((uint8_t) (pn9_shift[7] << 7 
        | pn9_shift[6] << 6 | pn9_shift[5] << 5 
        | pn9_shift[4] << 4 |  pn9_shift[3] << 3 |  pn9_shift[2] << 2
        | pn9_shift[1] << 1 |  pn9_shift[0]));


        // white_o[j] = 0;
        // for (int y = 0; y < 8; y++)
        // {
        //     white_o[j] = white_o[j] | ((uint8_t)(pn9_shift[y] << y))^((white_i[j]&(0x80 >> y))>>(7-y));
        // }
        

        // printf("w data %02x, pn %02x, w out %02x\n", white_i[j] , ((uint8_t) (pn9_shift[7] << 7 
        //     | pn9_shift[6] << 6 | pn9_shift[5] << 5 
        //     | pn9_shift[4] << 4 |  pn9_shift[3] << 3 |  pn9_shift[2] << 2
        //     | pn9_shift[1] << 1 |  pn9_shift[0])), 
        //     white_o[j]);
    }
}


void dsss2(uint8_t dsss_i, uint8_t* dsss_o0, uint8_t* dsss_o1){
    
        switch ((dsss_i & 0x0f))
        {
        case 0x0:
            *dsss_o1 = 0x00;
            break;
        case 0x1:
            *dsss_o1 = 0x03;
            break;
        
        case 0x2:
            *dsss_o1 = 0x0c;
            break;
        
        case 0x3:
            *dsss_o1 = 0x0f;
            break;
        
        case 0x4:
            *dsss_o1 = 0x30;
            break;
        case 0x5:
            *dsss_o1 = 0x33;
            break;
        
        case 0x6:
            *dsss_o1 = 0x3c;
            break;
        
        case 0x7:
            *dsss_o1 = 0x3f;
            break;
        case 0x8:
            *dsss_o1 = 0xc0;
            break;
        
        case 0x9:
            *dsss_o1 = 0xc3;
            break;
        
        case 0xa:
            *dsss_o1 = 0xcc;
            break;
        
        case 0xb:
            *dsss_o1 = 0xcf;
            break;
        
        case 0xc:
            *dsss_o1 = 0xf0;
            break;
        
        case 0xd:
            *dsss_o1 = 0xf3;
            break;
        
        case 0xe:
            *dsss_o1 = 0xfc;
            break;
        
        case 0xf:
            *dsss_o1 = 0xff;
            break;
        
        default:
            break;
        }

        
        switch ((dsss_i & 0xf0)>>4)
        {
        case 0x0:
            *dsss_o0 = 0x00;
            break;
        case 0x1:
            *dsss_o0 = 0x03;
            break;
        
        case 0x2:
            *dsss_o0 = 0x0c;
            break;
        
        case 0x3:
            *dsss_o0 = 0x0f;
            break;
        
        case 0x4:
            *dsss_o0 = 0x30;
            break;
        case 0x5:
            *dsss_o0 = 0x33;
            break;
        
        case 0x6:
            *dsss_o0 = 0x3c;
            break;
        
        case 0x7:
            *dsss_o0 = 0x3f;
            break;
        case 0x8:
            *dsss_o0 = 0xc0;
            break;
        
        case 0x9:
            *dsss_o0 = 0xc3;
            break;
        
        case 0xa:
            *dsss_o0 = 0xcc;
            break;
        
        case 0xb:
            *dsss_o0 = 0xcf;
            break;
        
        case 0xc:
            *dsss_o0 = 0xf0;
            break;
        
        case 0xd:
            *dsss_o0 = 0xf3;
            break;
        
        case 0xe:
            *dsss_o0 = 0xfc;
            break;
        
        case 0xf:
            *dsss_o0 = 0xff;
            break;
        
        default:
            break;
        }
        
        
}


void encode_payload2(uint8_t* in, uint8_t* out, int payloadsize){

    bool a0, a1;
    uint8_t x[16];
    uint8_t whit_out[payloadsize*2];
    uint8_t conv_out[payloadsize*2];

    reset_conv_reg();
    
    for (int i = 0; i < payloadsize; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            ((in[i]&(0x80>>j))>0)? conv_encoder(1, &a0, &a1) : conv_encoder(0, &a0, &a1);
            x[15-2*j]=a0;
            x[15-2*j-1]=a1;
        }
        conv_out[2*i] = x[15] << 7 | x[14] << 6 | x[13] << 5 | x[12] << 4 | x[11] << 3 
                | x[10] << 2 | x[9] << 1 | x[8]; 
        conv_out[2*i+1] = x[7] << 7 | x[6] << 6 | x[5] << 5 | x[4] << 4 | x[3] << 3 
                | x[2] << 2 | x[1] << 1 | x[0]; 
        
    }
    whiten_data(conv_out, whit_out, payloadsize*2);

    for (int ii = 0; ii < payloadsize*2; ii++)
    {
        dsss2(whit_out[ii], &out[2*ii], &out[2*ii+1]);
    }
    
    
}