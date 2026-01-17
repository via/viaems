#!/usr/bin/python3
import sys
import cbor2 as cbor
import json
from cobs import cobs
import zlib
import struct

def _deframe(frame):
    decoded = cobs.decode(frame)
    lengthbytes = decoded[0:2]
    crcbytes = decoded[-4:]
    pdu = decoded[2:-4]
    crc = struct.unpack("<I", crcbytes)[0]
    length = struct.unpack("<H", lengthbytes)[0]
    if len(pdu) != length:
        raise ValueError(f"Length mismatch: {len(pdu)} pdu but header is {length}")
    if zlib.crc32(pdu) != crc:
        raise ValueError("CRC failure")
    result = cbor.loads(pdu)
    return result

frame = b""
while True:
    b = sys.stdin.buffer.read(1)
    if b == b"\0":
        try:
            deframed = _deframe(frame)
            print(json.dumps(deframed))
        except ValueError as e:
            print(e)
        frame = b""
    else:
        frame += b

       
