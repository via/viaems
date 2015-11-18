PLATFORM?=attiny861

.ifmake check
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O0 -g
CFLAGS+= -D TICKRATE=20000000

.elif PLATFORM=="stm32f4-discovery"
CC=arm-none-eabi-gcc
CFLAGS=  -I. -std=c99 -Wall -O0 -Wextra -g
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4 
CFLAGS+= -I /opt/libopencm3/arm-none-eabi/include -DSTM32F4
CFLAGS+= -D TICKRATE=84000000 -D FASTMATH
LDFLAGS=  -L/opt/libopencm3/arm-none-eabi/lib
LDFLAGS+= -lopencm3_stm32f4 -fno-use-linker-plugin
LDFLAGS+= -T platforms/stm32f4-discovery.ld -nostartfiles
all: tfi

.elif PLATFORM="avr"
CC=avr-gcc
CFLAGS=-I. -mmcu=${PLATFORM} -std=c99 -Wall -O3 -Wextra
CFLAGS+= -D TICKTRATE=16000000
LDFLAGS=-lgcc
all: tfi.hex lfuse.hex hfuse.hex
.endif


tfi: decoder.o util.o tfi.o scheduler.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o scheduler.o decoder.o util.o ${PLATFORM}.o  ${LDFLAGS}


tfi.hex: tfi
	avr-objcopy -j .text -j .data -O ihex tfi tfi.hex

fuses.hex: tfi
	avr-objcopy -j .fuse -O ihex tfi fuses.hex --change-section-lma .fuse=0

lfuse.hex: fuses.hex
	srec_cat fuses.hex -Intel -crop 0x00 0x01 -offset  0x00 -O lfuse.hex -Intel

hfuse.hex: fuses.hex
	srec_cat fuses.hex -Intel -crop 0x01 0x02 -offset  -0x01 -O hfuse.hex -Intel

check: decoder.o util.o scheduler.o test/check.o
	${CC} ${CFLAGS} -o check check.o decoder.o util.o scheduler.o ${LDFLAGS}


clean:
	-rm *.o
	-rm tfi
	-rm test/*.o
	-rm check
	-rm *.hex
