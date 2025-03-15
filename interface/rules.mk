OBJS+= pb_encode.o \
			 pb_decode.o \
			 pb_common.o

DESTOBJS+= \
  $(OBJDIR)/types.pb.o \
  $(OBJDIR)/configs.pb.o 

INTOBJDIR=obj/interface

$(INTOBJDIR):
	mkdir -p $(INTOBJDIR)

VPATH+= contrib/nanopb $(INTOBJDIR)
PROTOC_OPTS= -D obj/interface -I interface 
CFLAGS+=-Icontrib/nanopb/ -I obj/interface

$(OBJDIR)/console.o: $(INTOBJDIR)/types.pb.h

$(INTOBJDIR)/types.pb.c $(INTOBJDIR)/types.pb.h: $(INTOBJDIR)/configs.pb.h interface/types.proto interface/types.options
	mkdir -p obj/interface
	contrib/nanopb/generator/nanopb_generator $(PROTOC_OPTS) interface/types.proto

$(INTOBJDIR)/configs.pb.c $(INTOBJDIR)/configs.pb.h: interface/configs.proto interface/configs.options
	mkdir -p obj/interface
	contrib/nanopb/generator/nanopb_generator $(PROTOC_OPTS) interface/configs.proto
