PLATFORM?=gd32f4
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
				controllers.o \
				sim.o \
				util.o

include targets/${PLATFORM}.mk

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)


GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-Isrc/ -Isrc/platforms/common -Wall -Wextra -ggdb -g3 -std=c11 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=-I${TINYCBOR_DIR}/src
LDFLAGS+= -lm -L${OBJDIR} -l:${TINYCBOR_LIB}

VPATH+=src src/platforms src/platforms/common
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}


$(OBJDIR)/%.o: %.s
	${AS} -c -o $@ $<

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/viaems.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/viaems.o ${LDFLAGS}

$(OBJDIR)/benchmark: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/benchmark.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/benchmark.o ${LDFLAGS}

${OBJDIR}/${TINYCBOR_LIB}:
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} clean
	$(MAKE) -C ${TINYCBOR_DIR} ${MAKEFLAGS} CC="${CC}" CFLAGS="${CFLAGS}" freestanding-pass=1 BUILD_SHARED=0
	cp ${TINYCBOR_DIR}/lib/${TINYCBOR_LIB} ${OBJDIR}

format:
	clang-format -i src/*.[ch] src/platforms/*.[ch] src/platforms/*/*.[ch] src/platforms/common/*.[ch]

lint:
	clang-tidy src/*.c -- ${CFLAGS}

benchmark: $(OBJDIR)/benchmark

clean:
	-rm ${OBJDIR}/*


.PHONY: clean lint format integration benchmark
