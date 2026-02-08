OBJS+= console.pb.o
VPATH+= src/proto/
CFLAGS+=-Isrc/proto -Icontrib/viapb

proto: src/proto/console.pb.c src/proto/console.pb.h py/viaems_proto/__init__.py

# Hardcode the protobuf dependency for known sources
src/config.c src/console.c: proto

src/proto/console.pb.c src/proto/console.pb.h: proto/console.fds proto/viapb.options
	python contrib/viapb/generate.py \
	       -f proto/console.fds            \
	       --options proto/viapb.options   \
	       -c src/proto/console.pb.c       \
	       -H src/proto/console.pb.h

proto/console.fds: proto/console.proto
	python -m grpc_tools.protoc --include_imports -I proto -o proto/console.fds proto/console.proto

py/viaems_proto:
	mkdir py/viaems_proto

py/viaems_proto/__init__.py: proto/console.proto py/viaems_proto
	python -m grpc_tools.protoc -I proto --python_out py/viaems_proto proto/console.proto

