PLATFORM?=stm32f4-discovery

.ifmake check || coverage
PLATFORM=test
.endif

GITDESC!=git describe --tags --dirty

CFLAGS+= -I ../tinycbor/src

.if ${PLATFORM} == "stm32f4-discovery"
CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJS=libssp.a libssp_nonshared.a
OBJCOPY=arm-none-eabi-objcopy
OPENCM3_DIR=../libopencm3
ARCH_CFLAGS= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4
CFLAGS+=  -I. -std=c99 -Wall -O0 -Wextra -g -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+= -D TICKRATE=4000000
CFLAGS+=  -L${OPENCM3_DIR}/lib -I${OPENCM3_DIR}/include -DSTM32F4
CFLAGS+= -fstack-protector-strong -fno-strict-aliasing
LDFLAGS+= -lopencm3_stm32f4 -lc -lm -lnosys -l:libtinycbor.a -L .
LDFLAGS+= -T platforms/stm32f4-discovery.ld -nostartfiles

# TODO: can we exclude console.c, or stop using double promotions there?
#CFLAGS+= -Wdouble-promotion

.elif ${PLATFORM} == "hosted"
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -O0 -ggdb -DSUPPORTS_POSIX_TIMERS
CFLAGS+= -D TICKRATE=1000000 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
CFLAGS+= -fsanitize=undefined -fsanitize=address -DGIT_DESCRIBE=\"${GITDESC}\"
LDFLAGS+= -lm -lrt -l:libtinycbor.a -L .

.elif ${PLATFORM} == "test"
CFLAGS!=pkg-config --cflags check
LDFLAGS!=pkg-config --libs check
CC=gcc
CFLAGS+= -I. -std=c99 -Wall -Wwrite-strings -O0 -g
CFLAGS+= -D TICKRATE=4000000 -DUNITTEST
CFLAGS+= -fprofile-arcs -ftest-coverage 
CFLAGS+= -fsanitize=undefined -fsanitize=address -DGIT_DESCRIBE=\"${GITDESC}\"
LDFLAGS+= -lm
.endif

CFLAGS+= ${ARCH_CFLAGS}

all: viaems

libtinycbor.a:
	(cd ../tinycbor/; make CC="${CC}" LDFLAGS="${ARCH_CFLAGS}" CFLAGS="${ARCH_CFLAGS}" freestanding-pass=1)
	cp ../tinycbor/lib/libtinycbor.a .

libssp.a:
	${AR} rcs libssp.a

libssp_nonshared.a:
	${AR} rcs libssp_nonshared.a

replay_data.o: replay_data.cbor
	${LD} -r --format binary -o replay_data.o replay_data.cbor  --entry 0

OBJS+=decoder.o util.o scheduler.o console.o calculations.o config.o table.o sensors.o stats.o tasks.o

viaems: ${OBJS} viaems.o replay_data.o platforms/${PLATFORM}.o libtinycbor.a
	${CC} ${CFLAGS} -o viaems viaems.o ${OBJS} ${PLATFORM}.o replay_data.o ${LDFLAGS}

viaems.dfu: viaems
	${OBJCOPY} -O binary viaems viaems.dfu

program: viaems.dfu
	dfu-util -D viaems.dfu -a 0 -s 0x8000000:leave

check: viaems
	./viaems

coverage: check
	./viaems
	gcovr -r .

format:
	clang-format -i *.c *.h platforms/*.c

lint:
	clang-tidy *.c -- -I . -D TICKRATE=1000000


clean:
	-rm *.o *.a
	-rm viaems
	-rm viaems.dfu
	-rm *.hex
	-rm *.gcov *.gcno *.gcda *.gmon
	(cd ../tinycbor; make clean)
