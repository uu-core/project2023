D0=
D1=
BAUD=
ECC=
FEC=
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

tag:
	cd baseband; python3 generate-backscatter-pio.py $(D0) $(D1) $(BAUD) ./backscatter.pio --twoAntennas
	mkdir -p ./baseband/build
	cd baseband/build; cmake .. -D USE_ECC=$(ECC) -D USE_FEC=$(FEC) -D USE_RETRANSMISSION=$(RETRANSMISSION); make
	cp baseband/build/pio_backscatter.uf2 $(MOUNT_PATH)

clean:
	rm -rf baseband/build/*
