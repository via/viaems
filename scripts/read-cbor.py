#!/usr/bin/python3
import sys
import cbor
import json

while True:
    data = cbor.load(sys.stdin.buffer)
    try:
        print(json.dumps(data))
    except BlockingIOError:
        pass
       
