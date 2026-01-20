from viaems.connector import SimConnector
import json
from google.protobuf import json_format

#conn = SimConnector("obj/hosted/proxy")
conn = SimConnector("obj/hosted/viaems")
conn.start()

config = conn.getconfig()
print(config.ByteSize())
print(json_format.MessageToJson(config))

conn.kill()
