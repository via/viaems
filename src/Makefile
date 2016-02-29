PLATFORM?=stm32f4-discovery

.ifmake check
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -Itest -std=c99 -Wall -O0 -g
CFLAGS+= -D TICKRATE=100000000 -DUNITTEST

.else
.if ${PLATFORM} == "stm32f4-discovery"
CC=arm-none-eabi-gcc
CFLAGS=  -I. -std=c99 -Wall -O0 -Wextra -g
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4
CFLAGS+= -I /opt/libopencm3/arm-none-eabi/include -DSTM32F4
CFLAGS+= -D TICKRATE=84000000 -D FASTMATH
LDFLAGS=  -L/opt/libopencm3/arm-none-eabi/lib
LDFLAGS+= -lopencm3_stm32f4 -fno-use-linker-plugin -lc -lnosys
LDFLAGS+= -T platforms/stm32f4-discovery.ld -nostartfiles
all: tfi

.elif ${PLATFORM} == "hosted"
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O0 -g
CFLAGS+= -D TICKRATE=1000000000 -D_POSIX_C_SOURCE=199309L -DTIMER64 -D_GNU_SOURCE
LDFLAGS+= -lrt
all: tfi
.endif
.endif


OBJS=decoder.o util.o scheduler.o console.o calculations.o config.o table.o sensors.o

tfi: ${OBJS} tfi.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o ${OBJS} ${PLATFORM}.o  ${LDFLAGS}


check: ${OBJS} test/check.o test/check_platform.o test/check_decoder.o test/check_scheduler.o
	${CC} ${CFLAGS} -o check check.o check_platform.o check_decoder.o check_scheduler.o ${OBJS} ${LDFLAGS}


clean:
	-rm *.o
	-rm tfi
	-rm test/*.o
	-rm check
	-rm *.hex
