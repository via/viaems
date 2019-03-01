PLATFORM?=stm32f4-discovery

.ifmake check || coverage
PLATFORM=test
.endif

.if ${PLATFORM} == "stm32f4-discovery"
CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
OBJS=libssp.a libssp_nonshared.a
OBJCOPY=arm-none-eabi-objcopy
OPENCM3_DIR=../libopencm3
CFLAGS=  -I. -std=c99 -Wall -O3 -Wextra -g -flto
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4
CFLAGS+= -D TICKRATE=4000000
CFLAGS+=  -L${OPENCM3_DIR}/lib -I${OPENCM3_DIR}/include -DSTM32F4
CFLAGS+= -fstack-protector-strong -fno-strict-aliasing
LDFLAGS+= -lopencm3_stm32f4 -lc -lm -lnosys -flto -L .
LDFLAGS+= -T platforms/stm32f4-discovery.ld -nostartfiles

.elif ${PLATFORM} == "hosted"
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O0 -ggdb -lm -lrt -DSUPPORTS_POSIX_TIMERS
CFLAGS+= -D TICKRATE=1000000 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
CFLAGS+= -fsanitize=undefined

.elif ${PLATFORM} == "test"
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -Itest -std=c99 -Wall -Wwrite-strings -O0 -g
CFLAGS+= -D TICKRATE=4000000 -DUNITTEST
CFLAGS+= -fprofile-arcs -ftest-coverage 
CFLAGS+= -fsanitize=undefined
LDFLAGS+= -lm
.endif

all: tfi

libssp.a:
		${AR} rcs libssp.a

libssp_nonshared.a:
		${AR} rcs libssp_nonshared.a

OBJS+=decoder.o util.o scheduler.o console.o calculations.o config.o table.o sensors.o stats.o tasks.o

tfi: ${OBJS} tfi.o platforms/${PLATFORM}.o
	${CC} ${CFLAGS} -o tfi tfi.o ${OBJS} ${PLATFORM}.o ${LDFLAGS}

tfi.dfu: tfi
	${OBJCOPY} -O binary tfi tfi.dfu

program: tfi.dfu
	dfu-util -D tfi.dfu -a 0 -s 0x8000000:leave

check: tfi
	./tfi

coverage: check
	./tfi
	gcovr -r .


clean:
	-rm *.o *.a
	-rm tfi
	-rm tfi.dfu
	-rm *.hex
	-rm *.gcov *.gcno *.gcda *.gmon
	-rm tfi.sock
