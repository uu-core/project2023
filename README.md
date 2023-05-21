# Group 1 

## FEC

This branch using BCH (26,16) to correct error

## Parameter


Payload size: 2 bytes.
After BCH, payload size is 6 bytes.

## Flow of transmission

1. Divide the origin packet into 7 small packets.
2. Backscattering
3. After receive, translate packet data (26 bits) into raw data (16 bits).
4. Check remainder of BCH algorithm, if remainder is larger than 0, do correction.

## How to analyze our received packet?

Please using "stats/analysis.ipynb" to check the result of our received data.
