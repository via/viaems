from viaems.connector import SimConnector
import json

conn = SimConnector("obj/hosted/viaems")
conn.start()

structure = conn.get(path=[])
print(json.dumps(structure, indent=2))

conn.kill()
