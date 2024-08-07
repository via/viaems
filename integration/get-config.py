from viaems import ViaemsWrapper
import json

conn = ViaemsWrapper("obj/hosted/viaems")
conn.start()

structure = conn.get(path=[])
print(json.dumps(structure, indent=2))

conn.kill()
