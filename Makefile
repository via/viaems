PLATFORM?=hosted
OBJDIR=obj/${PLATFORM}

CONFIGS_JSON = $(wildcard configs/*.json)
CONFIGS_CBOR = $(CONFIGS_JSON:.json=.cbor)

TINYCBOR_DIR=$(PWD)/tinycbor
TINYCBOR_LIB=libtinycbor.a

all: $(OBJDIR)/viaems $(OBJDIR)/benchmark ${CONFIGS_CBOR}

OBJS += calculations.o \
				config.o \
				console.o \
				decoder.o \
				scheduler.o \
				sensors.o \
				table.o \
				tasks.o \
				util.o \
				viaems.o

include targets/${PLATFORM}.mk

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)

GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-I src/ -Wall -Wextra -Werror -g -std=c99 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=-I ${TINYCBOR_DIR}/src -I${OBJDIR}
LDFLAGS+= -lm -L${OBJDIR} -l:${TINYCBOR_LIB}

OPENCM3_DIR=$(PWD)/libopencm3

VPATH=src src/platforms
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

%.cbor: %.json
	python3 scripts/json-to-cbor.py <$< >$@

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}

$(OBJDIR)/benchmark: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}

${OBJDIR}/${TINYCBOR_LIB}:
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} clean
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} CC="${CC}" CFLAGS="${CFLAGS}" freestanding-pass=1 BUILD_SHARED=0
	cp ${TINYCBOR_DIR}/lib/${TINYCBOR_LIB} ${OBJDIR}

format:
	clang-format -i src/*.c src/*.h src/platforms/*.c

lint:
	clang-tidy src/*.c -- ${CFLAGS}

benchmark: $(OBJDIR)/benchmark

clean:
	-rm ${OBJDIR}/*


.PHONY: clean lint format integration benchmark
