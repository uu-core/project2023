import os
import math
import argparse
from pathlib import Path
from xml.dom.minidom import parse, parseString

CLKFREQ = 125  # MHz
CARRIER_FREQ = 2450
RF_STUDIO_VALUE_LENGTH_HEX = 8
RF_STUDIO_SYMBOL_RATE_RATIO = 176720286/100000  # RF-studio does weird things

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

args = parser.parse_args()
d0 = args.divider0
d1 = args.divider1
b = args.baudrate

if (CLKFREQ*(10**6)) % args.baudrate != 0:
    b = round((CLKFREQ*(10**6)) / round((CLKFREQ*(10**6)) / args.baudrate))
    print(
        f"\nWARNING: a baudrate of {args.baudrate} Baud is not achievable with a {CLKFREQ} MHz clock.\nTherefore, the closest achievable baud-rate {b} Baud will be used.\n")

assert d0 % 2 == 0 and d0 >= 2, "d0 must be an even integer larger than 1"
assert d1 % 2 == 0 and d1 >= 2, "d1 must be an even integer larger than 1"
assert b > 0, "baud-rate can not be negative"

out_path = os.path.abspath(f"../stats/configs/{d0}_{d1}_{int(b / 1000)}k.xml")

fcenter = (CLKFREQ*1000/d0 + CLKFREQ*1000/d1)/2
frequency = CARRIER_FREQ + (fcenter / 1000)
fdeviation = abs(CLKFREQ*1000/d1 - fcenter)
bandwidth = (b/1000 + 2*fdeviation)

print("\nGenerated RF config settings:\n" + "\n".join([
    f"  - frequency 0 shift: {(CLKFREQ/d0):.3f} MHz       (1 period = {d0} cycles @ {CLKFREQ} MHz clock)",
    f"  - frequency 1 shift: {(CLKFREQ/d1):.3f} Mhz       (1 period = {d1} cycles @ {CLKFREQ} MHz clock)",
    f"  - center frequency shift: {(fcenter/1000):.3f} MHz",
    f"  - deviation from center : {fdeviation:.2f} kHz",
    f"  - baud-rate {b:.2f}",
    f"  - occupied bandwith: {bandwidth:.2f} kHz",
    f"",
    f"Saved settings to {out_path}",
]), end="\n\n")

print(to_rf_hex(int(b * RF_STUDIO_SYMBOL_RATE_RATIO)))
replacements = {
    "DUMP_FILE": out_path,
    "BANDWIDTH": to_rf_hex(int(bandwidth)),
    "BAUDRATE": to_rf_hex(int(b * RF_STUDIO_SYMBOL_RATE_RATIO)),
    "FREQUENCY": to_rf_hex(math.floor(frequency)),
    "FREQUENCY_FRAC": to_rf_hex(int((frequency % 1) * 10000)),
    "CENTER_FREQUENCY": to_rf_hex(math.floor(frequency)),
    "PACKET_COUNT": 100,
}

with open("./template_rf_config.xml", "r") as infile, open(out_path, "w") as outfile:
    for line in infile:
        for src, target in replacements.items():
            line = line.replace("{" + src + "}", str(target))
        outfile.write(line)
