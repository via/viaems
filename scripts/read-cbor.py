#!/usr/bin/python3
import sys
import cbor
import json

while True:
    try:
        data = cbor.load(sys.stdin.buffer)
      #  print(json.dumps(data))
        print(data["values"][-6])
    except KeyboardInterrupt:
        break
    except EOFError:
        break
    except:
        pass
       
