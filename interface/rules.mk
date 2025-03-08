OBJS+= pb_encode.o \
			 pb_common.o

DESTOBJS+= \
  $(OBJDIR)/types.pb.o \
  $(OBJDIR)/configs.pb.o 

INTOBJDIR=obj/interface

GENSRCS += \
					 $(INTOBJDIR)/types.pb.c \
					 $(INTOBJDIR)/types.pb.h \
					 $(INTOBJDIR)/configs.pb.c \
					 $(INTOBJDIR)/configs.pb.h

$(INTOBJDIR):
	mkdir -p $(INTOBJDIR)

VPATH+= contrib/nanopb interface
PROTOC_OPTS= -D $(INTOBJDIR)
DEPS += $(wildcard $(INTOBJDIR)/*.d)
CFLAGS+=-Icontrib/nanopb/ -I $(INTOBJDIR)


$(OBJDIR)/%.pb.o: %.pb.c
	${CC} -DBLEH ${CFLAGS} -MMD -c -o $@ $(INTOBJDIR)/interface/$<

%.pb.c %.pb.h: %.proto %.options $(INTOBJDIR)
	contrib/nanopb/generator/nanopb_generator $(PROTOC_OPTS)  $<

%.pb.c %.pb.h: %.proto $(INTOBJDIR)
	contrib/nanopb/generator/nanopb_generator $(PROTOC_OPTS)  $<
