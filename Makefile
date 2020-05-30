OBJDIR=arch/stm32f4

all: $(OBJDIR)/viaems

include stm32f4.mk

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

CFLAGS+=-I src/

OPENCM3_DIR=$(PWD)/libopencm3

VPATH=src src/platforms
DESTOBJS = $(addprefix ${OBJDIR}/, ${OBJS})


$(OBJDIR)/%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

$(OBJDIR)/viaems: ${DESTOBJS}
	${CC} -o $@ ${CFLAGS} ${DESTOBJS} ${LDFLAGS}

clean:
	-rm arch/stm32f4/*
