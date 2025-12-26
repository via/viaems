from viaems.connector import ViaemsWrapper
import json

conn = SimConnector()
conn.start()

structure = conn.get(path=[])
print(json.dumps(structure, indent=2))

conn.kill()
