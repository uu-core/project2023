#!/Users/wenya589/.pyenv/shims/python
#
# Copyright 2023, 2023 Wenqing Yan <yanwenqingindependent@gmail.com>
#
# This file is part of the pico backscatter project
# Analyze the communication systme performance with the metrics (time, reliability and distance).
import math
from io import StringIO
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from numpy import NaN
from pylab import rcParams
rcParams["figure.figsize"] = 16, 4

## WALSH GENERATION AND CODE ###################
def get_walsh_codes(order):
    #basic element(seed) of walsh code generator
    W = np.array([0])
    for i in range(order):
        W = np.tile(W, (2, 2))
        half = 2**i
        W[half:, half:] = np.logical_not(W[half:, half:])
    return W

# NOTE: These orders are based on the C code, DO NOT CHANGE!
walsh_combinations = [
    [0,0,0,0],
    [0,0,0,1],
    [0,0,1,0],
    [0,0,1,1],
    [0,1,0,0],
    [0,1,0,1],
    [0,1,1,0],
    [0,1,1,1],
    [1,0,0,0],
    [1,0,0,1],
    [1,0,1,0],
    [1,0,1,1],
    [1,1,0,0],
    [1,1,0,1],
    [1,1,1,0],
    [1,1,1,1],
]
walsh_sample_pos_combinations = [
    [0, 0],
    [0, 1],
    [1, 0],
    [1, 1],
    # Not used below
    [0, 0],
    [0, 0],
    [0, 0],
    [0, 0]
]

walsh_codes = get_walsh_codes(4)
sample_pos_walsh_codes = get_walsh_codes(3)
################################################

# Generating large comparision files takes a lot time.
# Since they only depend on the packet length, we can
# cache the output in a dict with the length as key.
cached_comparison_files = {}

# read the log file
def readfile(filename, USE_RETRANSMISSION=False):
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
        names=["time_rx", "frame", "rssi"]
    )
    df.dropna(inplace=True)
    # covert to time data type
    df.time_rx = df.time_rx.str.rstrip().str.lstrip()
    df.time_rx = pd.to_datetime(df.time_rx, format='%H:%M:%S.%f')
    for i in range(len(df)):
        df.iloc[i, 0] = df.iloc[i, 0].strftime("%H:%M:%S.%f")
    # parse the payload to seq and payload
    df.frame = df.frame.str.rstrip().str.lstrip()
    df = df[df.frame.str.contains("packet overflow") == False]
    df['seq'] = df.frame.apply(lambda x: int(x[3:5], base=16))
    if USE_RETRANSMISSION:
        df['rtx'] = df.frame.apply(lambda x: bin(int(x[12:14], base=16)).count("1") >= 4)
    else:
        df['rtx'] = df.frame.apply(lambda x: False)
    df['payload'] = df.frame.apply(lambda x: x[6:])
    # parse the rssi data
    df.rssi = df.rssi.str.lstrip().str.split(" ", expand=True).iloc[:, 0]
    df.rssi = df.rssi.astype('int')
    df = df.drop(columns=['frame'])
    df.reset_index(inplace=True)
    return df

# parse the hex payload, return a list with int numbers for each byte
def parse_payload(payload_string, USE_ECC=False, USE_FEC=False):
    if USE_ECC:
        # yes
        binary = list(
            map(lambda x: list(format(int(x, base=16), "0>8b")), payload_string.split()))
        flat_binary = [item for sublist in binary for item in sublist]
        bits = [flat_binary[i:i+3] for i in range(0, len(flat_binary), 3)]
        tmp = list(map(lambda x: 0 if x.count("0") > 1 else 1, bits))
        return list(map(lambda x: int("".join(str(c) for c in x), base=2), [tmp[i:i+8] for i in range(0, len(tmp), 8)]))
    elif USE_FEC:
        binary = list(
            map(lambda x: list(format(int(x, base=16), "0>8b")), payload_string.split()))
        flat_binary = [item for sublist in binary for item in sublist]

        # Multiply each code with the received data (dot product).
        # Get the Walsh code that is closest to the received bits
        values = []
        data = [int(bit) for bit in flat_binary[24:32]]
        for code in sample_pos_walsh_codes:
            values.append(np.dot(np.array(data), np.array(code)))
        sample_position = walsh_sample_pos_combinations[np.argmax(np.array(values))]

        values = []
        data = [int(bit) for bit in flat_binary[32:]]
        for code in walsh_codes:
            values.append(np.dot(np.array(data), np.array(code)))
        bits = walsh_combinations[np.argmax(np.array(values))]

        return [
            int("".join([str(b) for b in flat_binary[0:8]]), base=2),
            int("".join([str(b) for b in flat_binary[8:16]]), base=2),
            int("".join([str(b) for b in flat_binary[16:24]]), base=2),
            int("".join([str(b) for b in sample_position]), base=2),
            int("".join([str(b) for b in bits]), base=2)
        ]
    else:
        tmp = map(lambda x: int(x, base=16), payload_string.split())
        return list(tmp)

def popcount(n):
    return bin(n).count("1")

# compare the received frame and transmitted frame and compute the number of bit errors
def compute_bit_errors(seq, payload, sequence, PACKET_LEN=32, USE_FEC=False):
    if not USE_FEC:
        return sum(
            map(
                popcount,
                (
                    np.array(payload[:PACKET_LEN])
                    ^ np.array(sequence[:len(payload[:PACKET_LEN])])
                ),
            )
        )

    # Payload is only 1 byte once parsed
    sample_position = payload[1]
    data = payload[2]
    # Sequence is a list of 2 bytes for FEC (1 sample),
    # get the correct one based on the sample position (0-1 is left byte, 2-3 is right byte)
    sample_byte = sequence[0 if sample_position < 2 else 1]
    sample_data = (sample_byte >> (0 if (sample_position % 2) else 4)) & 0x0F
    errors = bin(data ^ sample_data).count("1")
    if errors > 0:
        seq_bin = format((sequence[0] << 8) + sequence[1], "0>16b")
        #print(f"seq: {seq}, payload[0]: {payload[0]}, sequence: {seq_bin}, Data: {data}, sample_pos: {sample_position}, errors: {errors}")
    return errors

# deal with seq field overflow problem, generate ground-truth sequence number
def replace_seq(df, MAX_SEQ):
    df['new_seq'] = None
    df_rtx = df[df["rtx"] == True]
    df = df[df["rtx"] == False]

    # Reset the indexing such that we dont get a mismatch between regular and rtx
    df_rtx.index = range(0, len(df_rtx))
    df.index = range(0, len(df))

    for x in [df, df_rtx]:
        count = 0
        if len(x) == 0 or len(x.seq) == 0:
            continue
        x.iloc[0, x.columns.get_loc('new_seq')] = x.seq[0]
        for idx in range(1, len(x)):
            if x.seq[idx] < x.seq[idx-1] - 50:
                count += 1
                # for the counter reset scanrio, replace the seq value with order
            x.iloc[idx, x.columns.get_loc('new_seq')] = MAX_SEQ*count + x.seq[idx]

    return (df, df_rtx)

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
    while (u1 == 0 or u2 == 0):
        seed = rnd(seed)
        u1 = np.float64(seed/0xFFFFFFFF)
        seed = rnd(seed)
        u2 = np.float64(seed/0xFFFFFFFF)
    tmp = 0x7FF * \
        np.float64(math.sqrt(np.float64(-2.0 * np.float64(math.log(u1)))))
    return np.trunc(max([0, min([0x3FFFFF, np.float64(np.float64(tmp * np.float64(math.cos(np.float64(two_pi * u2)))) + 0x1FFF)])])), seed

# generate the transmitted file for comparison
# generate a 40MB file, in case transmit too many data (larger than required 2MB)
TOTAL_NUM_16RND = 512*128 # Maximum size
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
def compute_ber(df, df_rtx, PACKET_LEN=32, MAX_SEQ=256, USE_ECC=False, USE_FEC=False):
    # Get a complete list of sequences received.
    # The idea is to fill in any gaps using the retransmitted packets (if any).
    seqs = df.merge(df_rtx, how="outer", sort=True, on="seq")["seq"]
    seqs = seqs.apply(lambda x: int(x))
    print(len(seqs))
    print(len(df))
    print(len(df_rtx))
    packets = len(seqs)
    first_seq = seqs[0]+1
    last_seq = seqs[packets-1]+1
    total_transmitted_packets = last_seq - first_seq

    # dataframe records the bit error for each packet, use the seq number as index
    error = pd.DataFrame(columns=["seq", "bit_error_tmp"])
    # seq number initialization
    # print(f"The total number of packets transmitted by the tag is {total_transmitted_packets}.")
    error.seq = range(first_seq, last_seq)
    # bit_errors list initialization
    error.bit_error_tmp = [list() for x in range(len(error))]
    # compute in total transmitted file size
    file_size = (len(error) - 1) * PACKET_LEN * 8
    if USE_FEC:
        # When using FEC, the packet is actually smaller than the packet length.
        file_size = (len(error) - 1) * 4

    # generate the correct file
    file_content = cached_comparison_files.get(PACKET_LEN)
    if file_content is None:
        file_content = generate_data(int(PACKET_LEN/2), TOTAL_NUM_16RND)
        cached_comparison_files[PACKET_LEN] = file_content

    misses = 0
    last_pseudoseq = 0  # record the previous pseudoseq

    # start counting the error bits
    for idx in range(packets):
        error_idxs = []
        if idx < len(df.seq):
            # return the matched row index for the specific seq number in log file
            error_data = error.index[error.seq == df.seq[idx]]
            if error_data.size != 0:
                error_idxs.append((df, error_data[0]))

        if idx < len(df_rtx.seq):
            error_data_rtx = error.index[error.seq == df_rtx.seq[idx]]
            if error_data_rtx.size != 0:
                error_idxs.append((df_rtx, error_data_rtx[0]))

        # Packet was not found in neither normal packets, nor RTX
        if len(error_idxs) == 0:
            continue

        for (df, error_idx) in error_idxs:
            # parse the payload and return a list, each element is 8-bit data, the first 16-bit data is pseudoseq
            payload = parse_payload(df.payload[idx], USE_ECC=USE_ECC, USE_FEC=USE_FEC)

            # TODO: Keep track of sample_position relative to pseudoseq in order to correct
            #       potential errors in the sample position byte.
            pseudoseq = (int(((payload[0] << 8) - 0) + payload[1]) % TOTAL_NUM_16RND)
            # deal with bit error in pseudoseq.
            if pseudoseq not in file_content.index:
                # TODO: Can also check with the sample index when using FEC in case
                # seq is corrupt as well.
                if (not USE_FEC) or (USE_FEC and (df.seq[idx] % 4) == 0):
                    pseudoseq = last_pseudoseq + PACKET_LEN
                else:
                    misses += 1
                    # Assume seq not corrupted
                    pseudoseq = (df.seq[idx] // 4) * 2

            # TODO: Attempt to fix the pseudoseq if corrupted
            #expected_seq = file_content.loc[(df.seq[idx] // 4)*2, 'data']
            #print(f"Expected sequence for {df.seq[idx]}: {format((expected_seq[0] << 8) + expected_seq[1], '0>16b')}")

            # compute the bit errors
            error.bit_error_tmp[error_idx].append(compute_bit_errors(df.seq[idx], payload[2:], file_content.loc[pseudoseq, 'data'], PACKET_LEN=PACKET_LEN, USE_FEC=USE_FEC))
            last_pseudoseq = pseudoseq

    # initialize the total bit error counter for entire file
    counter = 0
    bit_error = []
    # for the lost packet
    for l in error.bit_error_tmp:
        if l == []:
            if USE_FEC:
                tmp = 4
                counter += tmp # 4-bits of data transmitted per packet payload
            else:
                tmp = PACKET_LEN * 8
                counter += tmp  # when the seq number is lost, consider the entire packet payload as error
        else:
            tmp = min(l)
            counter += tmp  # when the seq number received several times, consider the minimum error
        bit_error.append(tmp)
    # update the bit_error column
    error['bit_error'] = bit_error
    # error = error.drop(columns='bit_error_tmp')
    # print("Error statistics dataframe is:")
    # print(error)
    #print(counter)

    # Calculate etx
    etx = total_transmitted_packets/packets
    return counter / file_size, error, etx, misses / total_transmitted_packets

# plot radar chart
def radar_plot(metrics, title=None):
    categories = ['Time', 'Reliability', 'Distance']
    system_ref = [62.321888, 0.201875*100, 39.956474923886844]
    system = [metrics[0], metrics[1], metrics[2]]

    label_loc = np.linspace(start=0.5 * np.pi, stop=11 /
                            6 * np.pi, num=len(categories))
    plt.figure(figsize=(8, 8))
    plt.subplot(polar=True)

    # please keep the reference for your plot, we will update the reference after each SR session
    plt.plot(np.append(label_loc, (0.5 * np.pi)), system_ref +
             [system_ref[0]], label='Reference', color='grey')
    plt.fill(label_loc, system_ref, color='grey', alpha=0.25)

    plt.plot(np.append(label_loc, (0.5 * np.pi)), system +
             [system[0]], label='Our system', color='#77A136')
    plt.fill(label_loc, system, color='#77A136', alpha=0.25)

    if title is None:
        plt.title('System Performance', size=20)
    else:
        plt.title(title, size=20)

    lines, labels = plt.thetagrids(np.degrees(
        label_loc), labels=categories, fontsize=18)
    plt.legend(fontsize=18, loc='upper right')
    plt.show()
