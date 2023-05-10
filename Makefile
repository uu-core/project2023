D0=
D1=
BAUD=
ECC=
MOUNT_PATH=/Volumes/RPI-RP2

ifdef ECC
ECC=ON
else
ECC=OFF
endif

tag:
	cd baseband; python3 generate-backscatter-pio.py $(D0) $(D1) $(BAUD) ./backscatter.pio --twoAntennas
	mkdir -p ./baseband/build
	cd baseband/build; cmake .. -D USE_ECC=$(ECC); make
	cp baseband/build/pio_backscatter.uf2 $(MOUNT_PATH)
