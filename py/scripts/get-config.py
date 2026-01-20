from viaems.connector import SimConnector
import json

conn = SimConnector("obj/hosted/viaems")
conn.start()

config = conn.getconfig()
print(config)

conn.kill()
