#!/usr/bin/env python3
import cbor
import json
import sys

blob = json.load(sys.stdin)
cbor.dump(blob, sys.stdout.buffer)
sys.stdout.flush()
