#!/usr/bin/env python3
import cbor2 as cbor
import json
import sys

blob = json.load(sys.stdin)
pdu = cbor.dumps(blob)
sys.stdout.buffer.write(pdu)
sys.stdout.buffer.flush()
