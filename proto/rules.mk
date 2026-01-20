OBJS+= console.pb.o
VPATH+= src/proto/
CFLAGS+=-Isrc/proto -Icontrib/viapb

proto: src/proto/console.pb.c src/proto/console.pb.h py/viaems/console_pb2.py

src/proto:
	mkdir src/proto

src/proto/console.pb.c src/proto/console.pb.h: proto/console.fds proto/viapb.options src/proto
	python contrib/viapb/generate.py \
	       -f proto/console.fds            \
	       --options proto/viapb.options   \
	       -c src/proto/console.pb.c       \
	       -H src/proto/console.pb.h

proto/console.fds: proto/console.proto
	python -m grpc_tools.protoc --include_imports -I proto -o proto/console.fds proto/console.proto

py/viaems/console_pb2.py: proto/console.proto
	python -m grpc_tools.protoc -I proto --python_out py/viaems proto/console.proto

