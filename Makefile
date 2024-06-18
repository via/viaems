PLATFORM?=stm32f4
OBJDIR=obj/${PLATFORM}

TINYCBOR_DIR=$(PWD)/contrib/tinycbor
TINYCBOR_LIB=libtinycbor.a


all: $(OBJDIR)/viaems # $(OBJDIR)/benchmark

OBJS += fiber.o \
				controllers.o \
				sim.o \
				console.process.o \
				test_loops.process.o \
				engine_mgmt.process.o


CONSOLE_OBJS += console.o

TEST_LOOPS_OBJS += test_loops.o

ENGINE_MGMT_OBJS += calculations.o \
										config.o \
										decoder.o \
										scheduler.o \
										sensors.o \
										table.o \
										util.o

include targets/${PLATFORM}.mk

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)


GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-Isrc/ -Isrc/platforms/common -Wall -Wextra -ggdb -g3 -std=c11 -DGIT_DESCRIBE=\"${GITDESC}\"
CFLAGS+=-I${TINYCBOR_DIR}/src -pipe
LDFLAGS+= -lm -L${OBJDIR} -l:${TINYCBOR_LIB}

VPATH+=contrib/uak contrib/uak/md src src/platforms src/platforms/common
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}


$(OBJDIR)/%.o: %.s
	${AS} ${ASFLAGS} -c -o $@ $<

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/viaems.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/viaems.o ${LDFLAGS}

$(OBJDIR)/benchmark: ${OBJDIR} ${DESTOBJS} ${OBJDIR}/${TINYCBOR_LIB} ${OBJDIR}/benchmark.o
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${OBJDIR}/benchmark.o ${LDFLAGS}

$(OBJDIR)/test_loops.process.o: ${TEST_LOOPS_OBJS}
	${CC} ${CFLAGS} -r -flinker-output=nolto-rel -o $@ ${TEST_LOOPS_OBJS}

$(OBJDIR)/engine_mgmt.process.o: ${ENGINE_MGMT_OBJS}
	${CC} ${CFLAGS} -r -flinker-output=nolto-rel -o $@ ${ENGINE_MGMT_OBJS}

$(OBJDIR)/console.process.o: ${CONSOLE_OBJS}
	${CC} ${CFLAGS} -r -flinker-output=nolto-rel -o $@ ${CONSOLE_OBJS}


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
