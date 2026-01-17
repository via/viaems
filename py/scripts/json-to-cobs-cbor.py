import cbor2 as cbor
from cobs import cobs
import json
import struct
import sys
import zlib

blob = json.load(sys.stdin)
pdu = cbor.dumps(blob)
crc = zlib.crc32(pdu)
lenbytes = struct.pack("<H", len(pdu))
crcbytes = struct.pack("<I", crc)
payload = lenbytes + pdu + crcbytes
sys.stdout.buffer.write(b"\0" + cobs.encode(payload) + b"\0")
sys.stdout.buffer.flush()
