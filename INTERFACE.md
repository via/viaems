# ViaEMS Wire Interface

Messages are sent and received from the EMS in a binary protocol consisting of
CBOR maps.  Some messages from the EMS are in direct response to requests, but
some are also sent on intervals. All messages in either direction have a `type`
field that describes the intent.

The map objects in either direction are packed against the previous message.
Multiple requests are simply sent concatenated, and messages from the EMS start
at after the last byte of the preceeding message.

Typical interval messages from the EMS include:
- `feed` messages, which include a list of values which represent status updates
  from the EMS.  These get sent as fast as possible from the EMS.
- `description` messages.  These list the fields that are included in a `feed`
  message.  It would be too expensive to render each status update message as a
  map of field to value, so the EMS sends the current list of fields at least
  once per second.  All `feed` messages are gauranteed to be in the order of the
  most recent `description` message.

All other messages from the EMS are a result of requests from a client.
Messages from the client are of type `request`, and responses to requests are of
type `response`.  Additionally, all requests carry an optional `id` integer
field, which the EMS will echo as an `id` field in the `response` reply.

## Interval Message Reference

### Feed Descriptions
This message type is produced at a regular interval, and contains a list of the
items that are presented in the `feed"`message type.  Each node is a map with a
name and a description of the node.

Example:
```
{
    "keys": [
        "cputime",
        "ve",
        "lambda",
        "fuel_pulsewidth_us",
        "temp_enrich_percent",
        "injector_dead_time",
        "accel_enrich_percent",
        "advance",
        "dwell",
        "sensor.map",
        "sensor.iat",
        "sensor.clt",
        "sensor.brv",
        "sensor.tps",
        "sensor.aap",
        "sensor.frt",
        "sensor.ego",
        "rpm",
        "sync",
        "rpm_variance",
        "t0_count",
        "t1_count"
    ],
    "type": "description"
}
```

### Feed update
This message type is produced at a regular interval, and contains the list of
values as described by the `description` message type.  It is gauranteed that any
`feed` message occuring after a `description` message will match the preceeding
`description` message's format.

```
{
    "type": "feed",
    "values": [
        0,
        0.0,
        0.0,
        0,
        0.0,
        0.0,
        0.0,
        0.0,
        0,
        50.0,
        10.0,
        50.0,
        13.800000190734863,
        25.0,
        50.0,
        15.0,
        0.6800000071525574,
        0,
        0,
        0.0,
        0,
        0
    ]
}
```

### Events
This message type is produced in response to trigger events, or changes in
scheduled outputs and gpios. All messages carry a timestamp and a value.  These
messages are produced if event logging is enabled.

```
{
    "type": "event",
    "time": 1140000,
    "event": {
        "type": "output",
        "outputs": 4
    }
}

{
    "type": "event",
    "time": 1140000,
    "event": {
        "type": "trigger",
        "pin": 0
    }
}
```

## Client Requests
All requests contain a `type` field of `request` and an optional `id` integer field, which can
be used to line up responses to a given request.  Responses to a response are
gauranteed to have a `type` field of `response`, an `id` field, and `success` boolean field.

### Ping
Used to ensure two-way connectivity.

Example request: 
```
{
    "type": "request",
    "method": "ping",
    "id": 6
}
```

Response:
```
{
    "type": "response", 
    "id": 6, 
    "success": true, 
    "response": "pong
}
```

### Structure
Returns the structure of the EMS configuration with all map leaves containing a
type and description field.  Each leaf in the structure map can be used in
subsequent operations as a path to `get` or `set`.  The response also contains a
`types` field that lists various additional types that can be used to defined
the configuration structure. If the `_type` of a field is not one of the
following primitive types, it should be listed in the `types` field, and
composed of other primitive types.

Example request:
```
    "id": 2,
    "method": "structure",
    "type": "request"
}
```

response (trimmed for display):
```
{
    "id": 2,
    "response": {
        "boost-control": {
            "overboost": {
                "_type": "float",
                "description": "High threshold for boost cut (kpa)"
            },
            "pin": {
                "_type": "uint32",
                "description": "GPIO pin for boost control output"
            },
            "pwm": {
                "_type": "table"
            },
            "threshold": {
                "_type": "float",
                "description": "Boost low threshold to enable boost control"
            }
        },
        "decoder": {
            "offset": {
                "_type": "float",
                "description": "offset past TDC for sync pulse"
            },
            "rpm_window_size": {
                "_type": "uint32",
                "description": "averaging window (N teeth)"
            },
            "type": {
                "_type": "string",
                "choices": [
                    "tfi",
                    "cam24+1"
                ],
                "description": "decoder wheel type"
            }
        },
        "sensors": {
            "aap": {
                "_type": "sensor"
            },
            "brv": {
                "_type": "sensor"
            },
            "clt": {
                "_type": "sensor"
            },
            "ego": {
                "_type": "sensor"
            },
            "frt": {
                "_type": "sensor"
            },
            "iat": {
                "_type": "sensor"
            },
            "map": {
                "_type": "sensor"
            },
            "tps": {
                "_type": "sensor"
            }
        },
        "outputs": [
            {
                "_type": "output"
            },
            {
                "_type": "output"
            },
            {
                "_type": "output"
            },
            {
                "_type": "output"
            }
        ]
    },
    "success": true,
    "type": "response",
    "types": {
        "output": {
            "angle": {
                "_type": "float",
                "description": "angle past TDC to trigger event"
            },
            "inverted": {
                "_type": "bool",
                "description": "inverted"
            },
            "pin": {
                "_type": "uint32",
                "description": "pin"
            },
            "type": {
                "_type": "string",
                "choices": [
                    "disabled",
                    "fuel",
                    "ignition"
                ],
                "description": "output type"
            }
        }
    }
}
```

#### Types
- `uint32` fields represent an unsigned integer
- `float` fields represent a single precision floating point value
- `bool` fields represent a boolean value
- `string` fields represent a text string, but frequently also represent an
  enumeration value. These fields will also contain a `choices` field that lists
  acceptable string values.
- `[float]` indicates that the field is a list of float values that would not be
  useful to individually describe.  These fields will also contain `len` field
  that indicates the maximum number of elements the list may have.
- `[[float]]` indicates that a field is a two dimension list of values, or a
  list of list of floats.  These fields contain a `len` field that indicates the
  maximum number of elements can be a `len`x`len` square.


### Get
Gets the full value of a config leaf entry. The specified path is a list that
represents the nesting structure of the desired entry.  Integer values are
interpreted as indexes into lists (for outputs or frequency inputs as an
example). Get paths can be as narrow or as wide as desired -- an empty path will
get the entire EMS configuration, assuming it can fit into a response. A path
that represents a specific integer or float value will return only that.

Example request (get second output configuration):
```
{
    "id": 2,
    "method": "get",
    "path": [
        "outputs",
        1
    ],
    "type": "request"
} 
```

Response:
```
{
    "id": 2,
    "response": {
        "angle": 120.0,
        "inverted": 0,
        "pin": 1,
        "type": "ignition"
    },
    "success": true,
    "type": "response"
}
```

### Set
Set parts of a config leaf entry's value. The specified path is a list that
represents the nesting structure of the desired entry.  Integer values are
interpreted as indexes into lists (for outputs or frequency inputs as an
example). The full value doesn't need to be specified: any valid names in the
value will be used to set the config appropriately. A set request will respond
with what a get request would for the same path, after attempting to set the
given values.

Example request:
```
{
    "id": 2,
    "method": "set",
    "path": [
        "outputs",
        1
    ],
    "type": "request",
    "value": {
        "inverted": 1,
        "pin": 19
    }
}
```

Response:
```
{
    "id": 2,
    "response": {
        "angle": 120.0,
        "inverted": 1,
        "pin": 19,
        "type": "ignition"
    },
    "success": true,
    "type": "response"
}
```

### flash
Save the active configuration into flash, such that it will survive a power
cycle.

**Currently this functionality should be not be used while the engine is running!**

Example request:
```
{
    "id": 2,
    "method": "flash",
}
```

Response:
```
{
    "id": 2,
    "success": true,
}
```

### Bootloader
Reboot the target, and enter DFU mode on startup.  In DFU mode, the device can
have firmware loaded with the dfu protocol, such as with a `make program`.


Example request:
```
{
    "id": 2,
    "method": "bootloader",
}
```

There is no reliable response, as the device is rebooted.
