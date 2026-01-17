VPATH=src/platforms/${PLATFORM}

OBJS+= hosted.o

#CFLAGS+= -DPLATFORM_HAS_NATIVE_MESSAGING -DHOSTED_UDP
CFLAGS+= -Og -DSUPPORTS_POSIX_TIMERS -Wno-error=unused-result
CFLAGS+= -D TICKRATE=4000000 -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
CFLAGS+= -fsanitize=undefined -fsanitize=address -pthread

LDFLAGS+= -lrt

run: ${OBJDIR}/viaems
	${OBJDIR}/viaems

proxy: ${OBJDIR}/proxy

${OBJDIR}/proxy: ${OBJDIR}/proxy.o
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} ${OBJDIR}/proxy.o -lusb-1.0

integration: ${OBJDIR}/viaems
	python3 py/integration-tests/smoke-tests.py
	python3 py/integration-tests/safety-tests.py
	python3 py/integration-tests/interface-tests.py

.PHONY: proxy run
