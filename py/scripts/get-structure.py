from viaems.connector import ViaemsWrapper
import json

conn = SimConnector()
conn.start()

structure = conn.structure()
print(json.dumps(structure, indent=2))

conn.kill()
