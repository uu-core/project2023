D0=
D1=
BAUD=
ECC=
FEC=
COMPRESSION=
MOUNT_PATH=/Volumes/RPI-RP2

ifdef ECC
ECC=ON
else
ECC=OFF
endif

ifdef FEC
FEC=ON
else
FEC=OFF
endif

ifdef RETRANSMISSION
RETRANSMISSION=ON
else
RETRANSMISSION=OFF
endif

ifdef COMPRESSION
COMPRESSION=ON
else
COMPRESSION=OFF
endif

tag:
	cd baseband; python3 generate-backscatter-pio.py $(D0) $(D1) $(BAUD) ./backscatter.pio --twoAntennas
	mkdir -p ./baseband/build
	cd baseband/build; cmake .. -D USE_ECC=$(ECC) -D USE_FEC=$(FEC) -D USE_RETRANSMISSION=$(RETRANSMISSION) -D USE_COMPRESSION=$(COMPRESSION); make
	cp baseband/build/pio_backscatter.uf2 $(MOUNT_PATH)

clean:
	rm -rf baseband/build/*
