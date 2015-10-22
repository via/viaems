PLATFORM?=attiny861

.ifmake check
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O0 -g
.else
CC=avr-gcc
CFLAGS=-I. -mmcu=${PLATFORM} -std=c99 -Wall -O2 -Wextra
LDFLAGS=-lgcc
.endif

all: tfi

tfi: decoder.o util.o tfi.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o decoder.o util.o ${PLATFORM}.o ${LDFLAGS}

check: decoder.o util.o test/check.o
	${CC} ${CFLAGS} -o check check.o decoder.o util.o ${LDFLAGS}


clean:
	-rm *.o
	-rm tfi
	-rm test/*.o
	-rm check
