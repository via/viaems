PLATFORM?=attiny861

.ifmake check
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O0 -g
.else
CC=avr-gcc
CFLAGS=-I. -mmcu=${PLATFORM} -std=c99 -Wall -O3 -Wextra
LDFLAGS=-lgcc
.endif

all: tfi.hex lfuse.hex hfuse.hex

tfi: decoder.o util.o tfi.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o decoder.o util.o ${PLATFORM}.o ${LDFLAGS}

tfi.hex: tfi
	avr-objcopy -j .text -j .data -O ihex tfi tfi.hex

fuses.hex: tfi
	avr-objcopy -j .fuse -O ihex tfi fuses.hex --change-section-lma .fuse=0

lfuse.hex: fuses.hex
	srec_cat fuses.hex -Intel -crop 0x00 0x01 -offset  0x00 -O lfuse.hex -Intel

hfuse.hex: fuses.hex
	srec_cat fuses.hex -Intel -crop 0x01 0x02 -offset  -0x01 -O hfuse.hex -Intel

check: decoder.o util.o test/check.o
	${CC} ${CFLAGS} -o check check.o decoder.o util.o ${LDFLAGS}


clean:
	-rm *.o
	-rm tfi
	-rm test/*.o
	-rm check
	-rm *.hex
