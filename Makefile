PLATFORM?=stm32f4
OBJDIR=obj/${PLATFORM}

TINYCBOR_DIR=$(PWD)/tinycbor
TINYCBOR_LIB=libtinycbor.a


all: $(OBJDIR)/viaems

include targets/${PLATFORM}.mk

OBJS += calculations.o \
				config.o \
				console.o \
				decoder.o \
				scheduler.o \
				sensors.o \
				stats.o \
				table.o \
				tasks.o \
				util.o \
				viaems.o

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)


GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-I src/ -Wall -Wextra -g -std=c99 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=-I ${TINYCBOR_DIR}/src
LDFLAGS+= -lm -L${OBJDIR} -l:${TINYCBOR_LIB}

OPENCM3_DIR=$(PWD)/libopencm3

VPATH=src src/platforms
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}


${OBJDIR}/${TINYCBOR_LIB}:
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} clean
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} CC="${CC}" CFLAGS="${CFLAGS}" freestanding-pass=1 BUILD_SHARED=0
	cp ${TINYCBOR_DIR}/lib/${TINYCBOR_LIB} ${OBJDIR}

format:
	clang-format -i src/*.c src/*.h src/platforms/*.c

lint:
	clang-tidy PLATFORM=hosted src/*.c -- ${CFLAGS}
	#-I . -D TICKRATE=1000000

clean:
	-rm ${OBJDIR}/*
