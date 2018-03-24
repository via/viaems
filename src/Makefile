PLATFORM?=stm32f4-discovery

.ifmake check || coverage
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -Itest -std=c99 -Wall -O0 -g
CFLAGS+= -D TICKRATE=4000000 -DUNITTEST
CFLAGS+= -fprofile-arcs -ftest-coverage 

.else
.if ${PLATFORM} == "stm32f4-discovery"
CC=arm-none-eabi-gcc
OPENCM3_DIR=../libopencm3
CFLAGS=  -I. -std=c99 -Wall -O3 -Wextra -g -flto
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4
CFLAGS+= -D TICKRATE=4000000
CFLAGS+=  -L${OPENCM3_DIR}/lib -I${OPENCM3_DIR}/include -DSTM32F4
LDFLAGS+= -lopencm3_stm32f4 -lc -lnosys -flto
LDFLAGS+= -T platforms/stm32f4-discovery.ld -nostartfiles
all: tfi

.elif ${PLATFORM} == "hosted"
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O2 -ggdb -lrt -DSUPPORTS_POSIX_TIMERS
CFLAGS+= -D TICKRATE=1000000 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
all: tfi
.endif
.endif


OBJS=decoder.o util.o scheduler.o console.o calculations.o config.o table.o sensors.o stats.o

tfi: ${OBJS} tfi.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o ${OBJS} ${PLATFORM}.o  ${LDFLAGS}


check: ${OBJS} test/check.o test/check_platform.o test/check_scheduler.o test/check_decoder.o
	${CC} ${CFLAGS} -o check check.o check_platform.o check_scheduler.o check_decoder.o  ${OBJS} ${LDFLAGS}

coverage: check
	./check
	gcovr -r .


clean:
	-rm *.o
	-rm tfi
	-rm test/*.o
	-rm check
	-rm *.hex
	-rm *.gcov *.gcno *.gcda
	-rm tfi.sock
