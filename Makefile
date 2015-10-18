PLATFORM=attiny861
CC=avr-gcc
CFLAGS=-I. -mmcu=${PLATFORM} -nostartfiles -nostdlib -std=c99 -Wall -O2
LDFLAGS=-lgcc

all: tfi

tfi: decoder.o util.o tfi.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o decoder.o util.o ${PLATFORM}.o ${LDFLAGS}


clean:
	-rm *.o
	-rm tfi
