PLATFORM?=stm32f4
OBJDIR=obj/${PLATFORM}

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
				viaems.o \
				replay_data.o

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)

TINYCBOR_DIR=$(PWD)/tinycbor
TINYCBOR_LIB=libtinycbor.a
OBJS+= ${TINYCBOR_LIB}

GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-I src/ -Wall -Wextra -g -std=c99 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=-I ${TINYCBOR_DIR}/src
LDFLAGS+= -lm -L${OBJDIR} -l:${TINYCBOR_LIB}

VPATH=src src/platforms
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}

replay_data.cbor:

${OBJDIR}/replay_data.o: replay_data.cbor
	if [ -f replay_data.cbor ]; then \
		${LD} -r --format binary -o ${OBJDIR}/replay_data.o replay_data.cbor  --entry 0; \
	else \
		${LD} -r --format binary -o ${OBJDIR}/replay_data.o /dev/null  --entry 0; \
	fi


$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}

${OBJDIR}/${TINYCBOR_LIB}:
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} clean
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} CC="${CC}" CFLAGS="${CFLAGS}" freestanding-pass=1 BUILD_SHARED=0
	cp ${TINYCBOR_DIR}/lib/${TINYCBOR_LIB} ${OBJDIR}

format:
	clang-format -i src/*.c src/*.h src/platforms/*.c

lint:
	clang-tidy src/*.c -- -I . -D TICKRATE=1000000

clean:
	-rm ${OBJDIR}/*
