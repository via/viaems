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
				viaems.o

DEPS = $(wildcard ${OBJDIR}/*.d)
-include $(DEPS)

GITDESC=$(shell git describe --tags --dirty)
CFLAGS+=-I src/ -Wall -Wextra -g -std=c99 -DGIT_DESCRIBE=\"${GITDESC}\"
LDFLAGS+= -lm

OPENCM3_DIR=$(PWD)/libopencm3

VPATH=src src/platforms
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})

$(OBJDIR):
	mkdir -p ${OBJDIR}

$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -MMD -c -o $@ $<

$(OBJDIR)/viaems: ${OBJDIR} ${DESTOBJS}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}

format:
	clang-format -i src/*.c src/*.h src/platforms/*.c

lint:
	clang-tidy src/*.c -- -I . -D TICKRATE=1000000

clean:
	-rm ${OBJDIR}/*
