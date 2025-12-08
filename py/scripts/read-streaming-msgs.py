#!/usr/bin/python3
import sys
import cbor2 as cbor
import json
from cobs import cobs
import zlib
import struct

def _deframe(frame):
    decoded = cobs.decode(frame)
    crcbytes = decoded[-4:]
    pdu = decoded[0:-4]
    crc = struct.unpack("<I", crcbytes)[0]
    if zlib.crc32(pdu) != crc:
        raise ValueError("CRC failure")
    result = cbor.loads(pdu)
    return result

frame = b""
while True:
    b = sys.stdin.buffer.read(1)
    if b == b"\0":
        print(json.dumps(_deframe(frame)))
        frame = b""
    else:
        frame += b

       
