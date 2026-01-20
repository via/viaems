from viaems.connector import SimConnector
import json
from google.protobuf import json_format
from viaems import console_pb2

#conn = SimConnector("obj/hosted/proxy")
conn = SimConnector("obj/hosted/viaems")
conn.start()

config = conn.getconfig()
print(json_format.MessageToJson(config))

conn.kill()
