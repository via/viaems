import sys
from interface import types_pb2
import struct

while True:
    readlen = sys.stdin.buffer.read(4)
    count = struct.unpack("I", readlen)[0]
    print(count)
    readdata = sys.stdin.buffer.read(count)
    msg = types_pb2.SensorsUpdate()
    msg.ParseFromString(readdata)
    print(msg)


