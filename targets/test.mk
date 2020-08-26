OBJS+= test.o

CFLAGS+= $(shell pkg-config --cflags check)
CFLAGS+= -O0 -ggdb
CFLAGS+= -D TICKRATE=4000000 -DUNITTEST
#CFLAGS+= -fsanitize=undefined -fsanitize=address

LDFLAGS+= $(shell pkg-config --libs check)

check: ${OBJDIR}/viaems
	${OBJDIR}/viaems
