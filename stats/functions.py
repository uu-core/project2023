#!/Users/wenya589/.pyenv/shims/python
#
# Copyright 2023, 2023 Wenqing Yan <yanwenqingindependent@gmail.com>
#
# This file is part of the pico backscatter project
# Analyze the communication systme performance with the metrics (time, reliability and distance).

from io import StringIO
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from numpy import NaN
from pylab import rcParams
rcParams["figure.figsize"] = 16, 4
import math


# read the log file
def readfile(filename):
    types = {
        "time_rx": str,
        "frame": str,
        "rssi": str,
    }
    df = pd.read_csv(
        StringIO(" ".join(l for l in open(filename))),
        skiprows=0,
        header=None,
        dtype=types,
        delim_whitespace=False,
        delimiter="|",
        on_bad_lines='warn',
        names = ["time_rx", "frame", "rssi"]
    )
    df.dropna(inplace=True)
    # covert to time data type
    df.time_rx = df.time_rx.str.rstrip().str.lstrip()
    df.time_rx = pd.to_datetime(df.time_rx, format='%H:%M:%S.%f')
    for i in range(len(df)):
        df.iloc[i,0] = df.iloc[i,0].strftime("%H:%M:%S.%f")
    # parse the payload to seq and payload
    df.frame = df.frame.str.rstrip().str.lstrip()
    df = df[df.frame.str.contains("packet overflow") == False]
    df['seq'] = df.frame.apply(lambda x: int(x[3:5], base=16))
    df['payload'] = df.frame.apply(lambda x: x[6:])
    # parse the rssi data
    df.rssi = df.rssi.str.lstrip().str.split(" ", expand=True).iloc[:,0]
    df.rssi = df.rssi.astype('int')
    df = df.drop(columns=['frame'])
    df.reset_index(inplace=True)
    return df


#binary to integer
def bin_int(new_array):
    int_list = [int("".join(map(str, new_array[i:i+8])), 2) for i in range(0, len(new_array), 8)]
    #print("decoded payload:", int_list)
    return int_list
#putting binary bits into array
def bin_to_array(binary_val):
    bit_array = [int(bit) for bit in binary_val]
    return bit_array
#parity bits calculation
def parity_calc(bits):
    r = 0
    flag = False
    while not flag:
        if 2 ** r >= bits + r + 1:
            flag = True
            return r
        else:
            r += 1

def parity_check(arry):
    arr1=arry[:32]
    #print("array1:",arr1)
    arr2=arry[32:]
    #print("array2:",arr2)
    bits = 24  #we encoded 3 bytes each
    x=0
    count=0
    err_count=0
    p_bits = parity_calc(bits)
    #print("p_bits:",p_bits)
    #print("before correction:",arry)
    
    del_pos = []
    pad_pos = [29,30,31,61,62,63]
    #array location of bits to be deleted that includes parity bits and padded bits
    for i in range(p_bits):
        del_pos.append(int(math.pow(2, i) - 1))
        del_pos.append(31+int(math.pow(2, i) - 1))
    del_pos = np.append(del_pos,pad_pos)
    del_pos = np.sort(del_pos)[::-1]
    
    #print("del_pos",del_pos)
    #index of the parity position and their corresponding data bits
    while(count<2): 
        array = []
        parity_array = []
        if count ==0:
            array = arr1
        else:
            array = arr2
        x=0
        while x<p_bits: 

            p_pos = int(math.pow(2, x) - 1)
            ctr = int(math.pow(2, x))
            #print("p_pos:", p_pos)
            index_arr = []  #stores the location of parity bit and its corresponding data bits
            data_array = []
            j = 0
            for i in range(p_pos, bits+p_bits):
                if ctr != 0:
                    index_arr.append(i)
                    j += 1
                    ctr -= 1
                    if ctr != 0:
                        i += 1
                else:
                    ctr = int(math.pow(2, x))
                    i = ctr + i + 1
            j_index = j
            #print("j_index:",j_index) 
            #print("index_array length:",len(index_arr))
            #print("index_arr:",index_arr)
            for i in range(j_index):
                #print("i:", i)
                #print("len(index_arr):", len(index_arr))
                #print("j_index:", j_index)
                #print("array:", array)
                #print("index_arr:", index_arr)
                data_array.append(array[index_arr[i]])
            parity_count = data_array.count(1)
            if parity_count%2 != 0:
                #print("even parity, no error")
                err_count+=1
            #else: 
                #err_count+=1
            parity_array.append(array[p_pos])
            x+=1
        
        #print("parity_array",parity_array)
        parity_array = parity_array[::-1]
        parity_dec = int(''.join(map(str, parity_array)), 2)
        
        
        #error correction
        if err_count !=0:
            #print("error count:",err_count)
            #print("Bit error at:",parity_dec)
            if array[parity_dec-1]==0:
                array[parity_dec-1]=1
                
            else:
                array[parity_dec-1]=0
            if count==0:
                arr1 = array
            else:
                arr2 = array
        count+=1
    new_array = arr1 + arr2
    #print("After correction:",new_array)
    
    #removing parity and padded bits
    for i in del_pos:
        del new_array[i]
    #print("After deletion:",new_array)
    new_list = bin_int(new_array)
    return new_list



    
    
    
#payload decoding
def parity_decode(payload_String):
   
    #for x in range(len(df)):
    #print("Payload string",payload_String)
    enc_list = parse_payload(payload_String)
        #print(enc_list)
    #if x == 0 :
    enc_payload = np.array(enc_list).reshape(1,len(enc_list))
    #else:
        #enc_payload = np.append(enc_payload,[enc_list],axis=0)
    #print("enc payload:", enc_payload)
    bin_array = np.vectorize(np.binary_repr)(enc_payload,8)
    #print("binary array: ",bin_array)
    combined_bin_array = np.apply_along_axis(lambda x: ''.join(x), axis=1, arr=bin_array)
    #for i in range(len(combined_bin_array)):
     #   arry = bin_to_array(combined_bin_array[i])
      #  parity_check(arry)
    binary_array = bin_to_array(combined_bin_array[0])
    #binary_array = np.vectorize(np.binary_repr)(combined_bin_array,1)
    #print("combined bin array ",combined_bin_array)
    #print("binary array:",binary_array)
    #print("length of binary array:",len(binary_array))
    result = parity_check(binary_array)
    return result
    #print('length of array')
    #print(len(arry))
          
           
# parse the hex payload, return a list with int numbers for each byte
def parse_payload(payload_string):
    tmp = map(lambda x: int(x, base=16), payload_string.split())
    return list(tmp)


def popcount(n):
    return bin(n).count("1")

# compare the received frame and transmitted frame and compute the number of bit errors
def compute_bit_errors(payload, sequence, PACKET_LEN=32):
    return sum(
        map(
            popcount,
            (
                np.array(payload[:PACKET_LEN])
                ^ np.array(sequence[: len(payload[:PACKET_LEN])])
            ),
        )
    )

# deal with seq field overflow problem, generate ground-truth sequence number
def replace_seq(df, MAX_SEQ):
    df['new_seq'] = None
    count = 0
    df.iloc[0, df.columns.get_loc('new_seq')] = df.seq[0]
    for idx in range(1,len(df)):
        if df.seq[idx] < df.seq[idx-1] - 50:
            count += 1
            # for the counter reset scanrio, replace the seq value with order
        df.iloc[idx, df.columns.get_loc('new_seq')] = MAX_SEQ*count + df.seq[idx]
    return df

# a 8-bit random number generator with uniform distribution
def rnd(seed):
    A1 = 1664525
    C1 = 1013904223
    RAND_MAX1 = 0xFFFFFFFF
    seed = ((seed * A1 + C1) & RAND_MAX1)
    return seed

# a 16-bit generator returns compressible 16-bit data sample
def data(seed):
    two_pi = np.float64(2.0 * np.float64(math.pi))
    u1 = 0
    u2 = 0
    while(u1 == 0 or u2 == 0):
        seed = rnd(seed)
        u1 = np.float64(seed/0xFFFFFFFF)
        seed = rnd(seed)
        u2 = np.float64(seed/0xFFFFFFFF)
    tmp = 0x7FF * np.float64(math.sqrt(np.float64(-2.0 * np.float64(math.log(u1)))))
    return np.trunc(max([0,min([0x3FFFFF,np.float64(np.float64(tmp * np.float64(math.cos(np.float64(two_pi * u2)))) + 0x1FFF)])])), seed

# generate the transmitted file for comparison
TOTAL_NUM_16RND = 512*40 # generate a 40MB file, in case transmit too many data (larger than required 2MB)
def generate_data(NUM_16RND, TOTAL_NUM_16RND):
    LOW_BYTE = (1 << 8) - 1
    length = int(np.ceil(TOTAL_NUM_16RND/NUM_16RND))
    index = [NUM_16RND*i*2 for i in range(length)]
    df = pd.DataFrame(index=index, columns=['data'])
    initial_seed = 0xabcd # initial seed
    pseudo_seq = 0 # (16-bit)
    seed  = initial_seed
    for i in index:
        payload_data = []
        for j in range(NUM_16RND):
            if pseudo_seq > 0xffff:
                pseudo_seq = 0
                seed = initial_seed
            pseudo_seq = pseudo_seq + 2
            number, seed = data(seed)
            payload_data.append((int(number) >> 8) - 0)
            payload_data.append(int(number) & LOW_BYTE)
        df.data[i] = payload_data
    return df

# main function to compute the BER for each frame, return both the error statistics dataframe and in total BER for the received data
def compute_ber(df, PACKET_LEN=32, MAX_SEQ=256):
    packets = len(df)

    # dataframe records the bit error for each packet, use the seq number as index
    error = pd.DataFrame(index=range(df.seq[0], df.seq[packets-1]+1), columns=['bit_error_tmp'])
    # seq number initialization
    print(f"The total number of packets transmitted by the tag is {df.seq[packets-1]+1}.")
    # bit_errors list initialization
    error.bit_error_tmp = [list() for x in range(len(error))]
    # compute in total transmitted file size
    file_size = len(error) * PACKET_LEN * 8
    # generate the correct file
    file_content = generate_data(int(PACKET_LEN/2), TOTAL_NUM_16RND)
    print(file_content)
    last_pseudoseq = 0 # record the previous pseudoseq
    # start count the error bits
    for idx in range(packets):
        # return the matched row index for the specific seq number in log file
        error_idx = df.seq[idx]
        # No packet with this sequence was received, the entire packet payload being considered as error (see below)
        if error_idx not in error.index:
            continue
        #parse the payload and return a list, each element is 8-bit data, the first 16-bit data is pseudoseq
        #payload = parse_payload(df.payload[idx])
        payload = parity_decode(df.payload[idx])
        pseudoseq = int(((payload[0]<<8) - 0) + payload[1])
        # deal with bit error in pseudoseq
        if pseudoseq not in file_content.index: pseudoseq = last_pseudoseq + PACKET_LEN
        # compute the bit errors
        error.bit_error_tmp[error_idx].append(compute_bit_errors(payload[2:], file_content.loc[pseudoseq, 'data'], PACKET_LEN=PACKET_LEN))
        last_pseudoseq = pseudoseq

    # initialize the total bit error counter for entire file
    counter = 0
    bit_error = []
    # for the lost packet
    for l in error.bit_error_tmp:
        if l == []:
            tmp = PACKET_LEN*8
            counter += tmp # when the seq number is lost, consider the entire packet payload as error
        else:
            tmp =  min(l)
            counter += tmp # when the seq number received several times, consider the minimum error
        bit_error.append(tmp)
    # update the bit_error column
    error['bit_error'] = bit_error
    # error = error.drop(columns='bit_error_tmp')
    print("Error statistics dataframe is:")
    print(error)
    return counter / file_size, error, file_content

# plot radar chart
def radar_plot(metrics):
    categories = ['Time', 'Reliability', 'Distance']
    system_ref = [62.321888, 0.201875*100, 39.956474923886844]
    system = [metrics[0], metrics[1], metrics[2]]

    label_loc = np.linspace(start=0.5 * np.pi, stop=11/6 * np.pi, num=len(categories))
    plt.figure(figsize=(8, 8))
    plt.subplot(polar=True)

    # please keep the reference for your plot, we will update the reference after each SR session
    plt.plot(np.append(label_loc, (0.5 * np.pi)), system_ref+[system_ref[0]], label='Reference', color='grey')
    plt.fill(label_loc, system_ref, color='grey', alpha=0.25)

    plt.plot(np.append(label_loc, (0.5 * np.pi)), system+[system[0]], label='Our system', color='#77A136')
    plt.fill(label_loc, system, color='#77A136', alpha=0.25)
    plt.title('System Performance', size=20)

    lines, labels = plt.thetagrids(np.degrees(label_loc), labels=categories, fontsize=18)
    plt.legend(fontsize=18, loc='upper right')
    plt.show()


        
    
    
    