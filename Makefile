PLATFORM?=stm32f4
OBJDIR=obj/${PLATFORM}

TINYCBOR_DIR=$(PWD)/contrib/tinycbor
TINYCBOR_LIB=libtinycbor.a


all: $(OBJDIR)/viaems $(OBJDIR)/benchmark

OBJS += calculations.o \
				config.o \
				console.o \
				decoder.o \
				scheduler.o \
				sensors.o \
				table.o \
				tasks.o \
				sim.o \
				util.o

include targets/${PLATFORM}.mk

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)


GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-I src/ -Wall -Wextra -g -std=c11 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=-I ${TINYCBOR_DIR}/src
LDFLAGS+= -lm -L${OBJDIR} -l:${TINYCBOR_LIB}

OPENCM3_DIR=$(PWD)/contrib/libopencm3

VPATH=src src/platforms src/platforms/${PLATFORM}
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/%.o: %.s
	${AS} -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/viaems.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/viaems.o ${LDFLAGS}

$(OBJDIR)/benchmark: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/benchmark.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/benchmark.o ${LDFLAGS}

${OBJDIR}/${TINYCBOR_LIB}:
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} clean
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} CC="${CC}" CFLAGS="${CFLAGS}" freestanding-pass=1 BUILD_SHARED=0
	cp ${TINYCBOR_DIR}/lib/${TINYCBOR_LIB} ${OBJDIR}

format:
	clang-format -i src/*.c src/*.h src/platforms/*.c src/platforms/*/*.c

lint:
	clang-tidy src/*.c -- ${CFLAGS}

benchmark: $(OBJDIR)/benchmark

clean:
	-rm ${OBJDIR}/*

.PHONY: clean lint format integration benchmark
