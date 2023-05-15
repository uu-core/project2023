import os
import math
import argparse
from pathlib import Path
from xml.dom.minidom import parse, parseString

CLKFREQ = 125  # MHz
CARRIER_FREQ = 2450  # MHz
RF_STUDIO_SYMBOL_RATE_RATIO = 176720286/100000  # baud in Bd
RF_STUDIO_FREQUENCY_FRAC_RATIO = 39117/5972 # offset in MHz
RF_STUDIO_DEVIATION_RATIO = 3201/100 # deviation is in kHz
RF_STUDIO_BANDWIDTH_INDEX_OFFSET = 64

def to_rf_hex(value):
    return f"0x{format(value, '08x')}"

parser = argparse.ArgumentParser(prog="RF-config XML generator",
                                 description="G3 WCNES rf studio config generator\nusage example: python3 generate-rf_config.py 20 18 100000")
parser.add_argument("divider0", type=int,
                    help=f"clock divider @ {CLKFREQ} MHz for frequency 0 shift; must be an even number e.g. 20 for {(CLKFREQ/20):.3f} MHz")
parser.add_argument("divider1", type=int,
                    help=f"clock divider @ {CLKFREQ} MHz for frequency 1 shift; must be an even number e.g. 18 for {(CLKFREQ/18):.3f} Mhz")
parser.add_argument("baudrate", type=int,
                    help="baud-rate [baud] e.g. 100000 for 100kBaud")
parser.add_argument("--ecc", action='store_true', default=False,
                    help="If you are using Error Code Correction")
parser.add_argument("--fec", action='store_true', default=False,
                    help="If you are using Walsh code Error Correction")

args = parser.parse_args()
d0 = args.divider0
d1 = args.divider1
b = args.baudrate
ecc = args.ecc
fec = args.fec

if (CLKFREQ*(10**6)) % args.baudrate != 0:
    b = round((CLKFREQ*(10**6)) / round((CLKFREQ*(10**6)) / args.baudrate))
    print(
        f"\nWARNING: a baudrate of {args.baudrate} Baud is not achievable with a {CLKFREQ} MHz clock.\nTherefore, the closest achievable baud-rate {b} Baud will be used.\n")

assert d0 % 2 == 0 and d0 >= 2, "d0 must be an even integer larger than 1"
assert d1 % 2 == 0 and d1 >= 2, "d1 must be an even integer larger than 1"
assert b > 0, "baud-rate can not be negative"

suffix = ""
if ecc:
    suffix = "_ecc"
elif fec:
    suffix = "_fec"
out_path = os.path.abspath(f"../stats/configs/{d0}_{d1}_{int(b / 1000)}k{suffix}.xml")

fcenter = (CLKFREQ*1000/d0 + CLKFREQ*1000/d1)/2
fdeviation = abs(CLKFREQ*1000/d1 - fcenter)
frequency = CARRIER_FREQ + (fcenter / 1000)
bandwidth = (b/1000 + 2*fdeviation)
bandwidth_index = 0
if ecc:
    packet_len = 40
elif fec:
    packet_len = 7
else
    packet_len = 16

available_bandwidths = [
    4.3,  # 64
    4.9,
    6.1,
    7.4,
    8.5,
    9.7,
    12.2,
    14.7,
    17.1,
    19.4,
    24.5,
    29.4,
    34.1,
    38.9,
    49.0,
    58.9,
    68.3,
    77.7,
    98.0,
    117.7,
    136.6,
    155.4,
    195.9,
    235.5,
    273.1,
    310.8,
    391.8,
    470.9,
    546.3,
    621.6,
    783.6,
    941.8,  # 95
    1092.5,  # 96
    1243.2,
    1567.2,
    1883.7,
    2185.1,
    2486.5,
    3134.4,
    3767.4
]

# RF Studio only accepts specific bandwidths
for idx, x in enumerate(available_bandwidths):
    if x > bandwidth:
        rf_bandwidth = RF_STUDIO_BANDWIDTH_INDEX_OFFSET + idx
        break

if rf_bandwidth == 0:
    print(f"Bandwidth of {bandwidth} is too large!")
    exit(1)

print("\nGenerated RF config settings:\n" + "\n".join([
    f"  - frequency 0 shift: {(CLKFREQ/d0):.3f} MHz       (1 period = {d0} cycles @ {CLKFREQ} MHz clock)",
    f"  - frequency 1 shift: {(CLKFREQ/d1):.3f} Mhz       (1 period = {d1} cycles @ {CLKFREQ} MHz clock)",
    f"  - center frequency shift: {(fcenter/1000):.3f} MHz",
    f"  - deviation from center: {fdeviation:.2f} kHz",
    f"  - baud-rate {b:.2f}",
    f"  - occupied bandwith: {bandwidth:.2f} kHz",
    f"  - packet Length: {packet_len}",
    f"",
    f"Saved settings to {out_path}",
]), end="\n\n")

replacements = {
    "BANDWIDTH": to_rf_hex(rf_bandwidth),
    "BAUDRATE": to_rf_hex(int(b * RF_STUDIO_SYMBOL_RATE_RATIO)),
    "FREQUENCY": to_rf_hex(math.floor(frequency)),
    "FREQUENCY_FRAC": to_rf_hex(int((frequency % 1) * 10000 * RF_STUDIO_FREQUENCY_FRAC_RATIO)),
    "CENTER_FREQUENCY": to_rf_hex(math.floor(frequency)),
    "DEVIATION": to_rf_hex(int(fdeviation * RF_STUDIO_DEVIATION_RATIO)),
    "PACKET_LENGTH": to_rf_hex(packet_len),
    "PACKET_COUNT": 500,
}

with open("./template_rf_config.xml", "r") as infile, open(out_path, "w") as outfile:
    for line in infile:
        for src, target in replacements.items():
            line = line.replace("{" + src + "}", str(target))
        outfile.write(line)
