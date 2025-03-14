import sys
from interface import types_pb2
import struct
from cobs import cobs
from crccheck.crc import Crc32

def parse(msg):
    decoded = cobs.decode(msg)
    length = struct.unpack("I", decoded[0:4])[0]
    crc = struct.unpack("I", decoded[4:8])[0]
    calcrc = Crc32.calc(decoded[8:])
    print(f"{length} {crc:x} {calcrc:x} {len(decoded[8:])}")
#    msg = types_pb2.SensorsUpdate()
    msg = types_pb2.Response()
    msg.ParseFromString(decoded[8:])
    print(f"msg: {msg}")




buf = b''
while True:
    nextbyte = sys.stdin.buffer.read(1)
    if nextbyte == b'':
        break

    if nextbyte == b'\0':
        if len(buf) == 0:
            break
        parse(buf)
        buf = b''
    else:
        buf += nextbyte

