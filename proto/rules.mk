OBJS+= console.pb.o
VPATH+= src/proto/
CFLAGS+=-Isrc/proto -Icontrib/viapb

src/proto/console.pb.c src/proto/console.pb.h: proto/console.fds
	python contrib/viapb/generate.py \
	       -f proto/console.fds            \
	       --options proto/viapb.options   \
	       -c src/proto/console.pb.c       \
	       -H src/proto/console.pb.h

proto/console.fds: proto/console.proto
	python -m grpc_tools.protoc -I proto -o proto/console.fds proto/console.proto

