PLATFORM?=gd32f4
OBJDIR=obj/${PLATFORM}

all: $(OBJDIR)/viaems $(OBJDIR)/benchmark

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
				tasks.o \
				sim.o \
				util.o \
				${TINYCBOR_OBJS}


include targets/${PLATFORM}.mk

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)


GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-Isrc/ -Isrc/platforms/common -Wall -Wextra -ggdb -g3 -std=c11 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=${TINYCBOR_CFLAGS}
LDFLAGS+= -lm -L${OBJDIR}

OPENCM3_DIR=$(PWD)/contrib/libopencm3

VPATH+=src src/platforms src/platforms/common contrib/tinycbor/src
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/viaems.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/viaems.o ${LDFLAGS}

$(OBJDIR)/benchmark: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/benchmark.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/benchmark.o ${LDFLAGS}

format:
	clang-format -i src/*.[ch] src/platforms/*.[ch] src/platforms/*/*.[ch] src/platforms/common/*.[ch]

lint:
	clang-tidy src/*.c -- ${CFLAGS}

benchmark: $(OBJDIR)/benchmark

clean:
	-rm ${OBJDIR}/*


.PHONY: clean lint format integration benchmark
