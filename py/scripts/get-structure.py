from viaems.connector import SimConnector
import json

conn = SimConnector("obj/hosted/viaems")
conn.start()

structure = conn.structure()
print(json.dumps(structure, indent=2))

conn.kill()
