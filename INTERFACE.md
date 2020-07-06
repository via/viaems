From EMS:
{"type": "desc", "nodes": [{Node}]}
This message type is produced at a regular interval, and contains a list of the
items that are presented in the "feed" message type.  Each node is a map with a
name and a description of the node.

<- {"type": "desc", "nodes": 
     [
       {"name": "rpm", "description": "engine speed"},
       {"name": "fuelcut", "description": "injector scheduling disabled"},
       {"name": "idt", "description": "injector dead time (ms)"}
     ]
   }

{"type": "feed": "values":[...]}
This message type is produced at a regular interval, and contains the list of
values as described by the "desc" message type.  It is gauranteed that any
"feed" message occuring after a "desc" message will match the preceeding "desc"
message's format.

<- {"type": "feed", "values": [ 3000.0, false, 1125 ]}

{"type": "response": "id": int, "success": bool, "value": {}}
Generic response to a request to the EMS.  If "success" is false, the value will
contain an "error" field describing what failed. Otherwise the value is
dependent on the request type.

<- {"type": "response", "success": false, "value":
     {"error": "unknown method"
   }

To EMS:
{"method": "structure": "id": int}
Returns the toplevel structure of the EMS configuration in the form of a map.
Each leaf in the structure map can be used in subsequent operations as a path.
It is planned for the leaf entries to contain information about schema and
options, but for now an empty map is sufficient to represent a leaf entry.


Types:
{
  "_id": "DecoderConfig",
  "_fields": {
    "type":{ "_type": "string", "choices": ["cam24+1", "tfi"]},
    "offset":{ "_type": "float"},
    "trigger_max_rpm_change": {"_type": "float"},
  }

  {"_id": "TableConfig",
    "_fields": {
      "title": {"_type": "string"},
      "num_axis": {"_type": "int"},
      "data": {"_type": "list"},
    }
  }
   


Example:
-> {"method": "structure": "id": 5}
<- {"type": "response", "id": 5, "success": true, "value":
     {"config": 
       "outputs": [{}, {}, {}, {}],
       "decoder": {},
       "freq": [{}, {}, {}, {}],
       "sensors": {"iat": {}, "clt": {}},
       "tables": {"ve": {}, "commanded_lambda": {}},
       "ignition": {},
       "fueling": {},
       "boostcontrol": {},
       "cel": {}
     }
   }

{"method": "get": "id": int, "path": []}
Gets the full value of a config leaf entry. The specified path is a list that
represents the nesting structure of the desired entry.  Integer values are
interpreted as indexes into lists (for outputs or frequency inputs as an
example).

example:
-> {"method": "get", "id": 6, "path": ["config", "freq", 0]}
<- {"type": "response", "id": 6, "path": ["config", "freq", 0], "value":
     {
       "type": "trigger",
       "edge": "rising"
     }
  }

{"method": "set": "id": int, "path": [], "value": {}}
Sets parts of a config leaf entry's value. The specified path is a list that
represents the nesting structure of the desired entry.  Integer values are
interpreted as indexes into lists (for outputs or frequency inputs as an
example). The full value doesn't need to be specified: any valid names in the
value will be used to set the config appropriately.  It is planned to allow the
path to refer to parts inside of a leaf entry, for example to set a specific
table value, but currently the entire top-level fields of a config entry must be
set at once (e.g. an entire table's values)
 

example:
{"method": "set", "id": 1, "path": ["config", "outputs", 2], "value":
  {
    "type": "fuel",
    "pin": 10,
  },
}

{"type": "response":, "id": 1, "path": ["config", "outputs", 2], "value": 
  {
    "type": "fuel",
      "pin": 10,
      "angle": 120,
  }
}

