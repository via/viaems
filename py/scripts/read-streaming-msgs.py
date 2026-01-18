#!/usr/bin/env python
import sys
import json
from cobs import cobs
import zlib
import struct

from google.protobuf import json_format 

import viaems.console_pb2

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
    message = viaems.console_pb2.Message()
    message.ParseFromString(pdu)
    return message

frame = b""
while True:
    b = sys.stdin.buffer.read(1)
    if b == b"\0":
        deframed = _deframe(frame)
        print(json.dumps( json_format.MessageToDict(deframed)))
        frame = b""
    else:
        frame += b

       
