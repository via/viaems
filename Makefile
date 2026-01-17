PLATFORM?=gd32f4
OBJDIR=obj/${PLATFORM}
BENCH?=0

all: $(OBJDIR)/viaems

TINYCBOR_OBJS= cborerrorstrings.o \
               cborencoder.o \
               cborparser.o

TINYCBOR_CFLAGS= -Icontrib/tinycbor/src

OBJS += calculations.o \
				config.o \
				console.o \
				decoder.o \
				scheduler.o \
				sensors.o \
				table.o \
				sim.o \
				tasks.o \
				util.o \
				stream.o \
				crc.o \
				benchmark.o \
				viaems.o \
				${TINYCBOR_OBJS}


include targets/${PLATFORM}.mk

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)


GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-Isrc/ -Isrc/platforms/common -Wall -Wextra -ggdb -g3 -std=c11 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=${TINYCBOR_CFLAGS}
LDFLAGS+= -lm -L${OBJDIR}

ifeq "$(BENCH)" "1"
	CFLAGS+=-DBENCHMARK=1
endif

VPATH+=src src/platforms src/platforms/common contrib/tinycbor/src
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}


$(OBJDIR)/%.o: %.s
	${AS} -c -o $@ $<

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}

format:
	clang-format -i src/*.[ch] src/platforms/*.[ch] src/platforms/*/*.[ch] src/platforms/common/*.[ch]

lint:
	clang-tidy src/*.c -- ${CFLAGS}

clean:
	-rm ${OBJDIR}/*


.PHONY: clean lint format integration benchmark
