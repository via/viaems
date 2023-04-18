OBJS+= hosted.o

CFLAGS+= -Og -DSUPPORTS_POSIX_TIMERS -Wno-error=unused-result
CFLAGS+= -D TICKRATE=4000000 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
CFLAGS+= -fsanitize=undefined -fsanitize=address -pthread


LDFLAGS+= -lrt

run: ${OBJDIR}/viaems
	${OBJDIR}/viaems

integration: ${OBJDIR}/viaems
	python3 integration/interface-tests.py
