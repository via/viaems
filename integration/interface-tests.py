#!/usr/bin/env python3
import unittest
import cbor
import subprocess
import random

class ViaemsWrapper:
    def __init__(self, binary):
        self.process = subprocess.Popen([binary], bufsize=0,
                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL)

    def send(self, payload):
        binary = cbor.dumps(payload)
        self.process.stdin.write(binary)

    def recv(self):
        result = cbor.load(self.process.stdout)
        return result

    def recv_until_id(self, id, max_messages=100000):
        while True:
            result = cbor.load(self.process.stdout)
            if 'id' in result and int(result['id']) == id:
                return result
            max_messages -= 1
            if max_messages == 0:
                return None

    def structure(self):
        id = random.randint(0, 1024)
        self.send({
            "id": id,
            "type": "request",
            "method": "structure",
            })
        result = self.recv_until_id(id)
        return result

    def get(self, path):
        id = random.randint(0, 1024)
        self.send({
            "id": id,
            "type": "request",
            "method": "get",
            "path": path,
            })
        result = self.recv_until_id(id)
        return result

def _leaves_have_types(obj):
    if type(obj) == dict:
        if '_type' in obj:
            return (type(obj['_type'] == str))
        return all([_leaves_have_types(x) for x in obj.values()])
    elif type(obj) == list:
        return all([_leaves_have_types(x) for x in obj])
    else:
        return False

def _leaves_have_descriptions(obj):
    if type(obj) == dict:
        if 'description' in obj:
            return (type(obj['description'] == str))
        return all([_leaves_have_types(x) for x in obj.values()])
    elif type(obj) == list:
        return all([_leaves_have_types(x) for x in obj])
    else:
        return False


class ViaemsInterfaceTests(unittest.TestCase):

    def test_structure_full(self):
        v = ViaemsWrapper("obj/hosted/viaems")
        result = v.structure()
        assert(result['success'])
        assert("response" in result)

        assert(_leaves_have_types(result['response']))

        outputs = result['response']['outputs']
        assert(len(outputs) == 24)
        assert(outputs[0]['_type'] == 'output')

        sensors = result['response']['sensors']
        assert(len(sensors) == 8)
        assert(sensors['iat']['_type'] == 'sensor')

    def test_structure_types(self):
        v = ViaemsWrapper("obj/hosted/viaems")
        result = v.structure()
        assert(result['success'])
        assert("types" in result)

        sensor = result['types']['sensor']
        assert(_leaves_have_descriptions(sensor))
        assert(_leaves_have_types(sensor))
        output = result['types']['output']
        assert(_leaves_have_descriptions(output))
        assert(_leaves_have_types(output))

    def test_get_full(self):
        v = ViaemsWrapper("obj/hosted/viaems")
        result = v.get(path=[])
        assert(result['success'])

        outputs = result['response']['outputs']
        assert(type(outputs[1]['angle']) == float)
        assert(type(outputs[1]['pin']) == int)
        
        sensors = result['response']['sensors']
        assert(type(sensors['map']['pin']) == int)
        assert(type(sensors['map']['method']) == str)
        assert(type(sensors['map']['range-min']) == float)

    def test_all_subpaths(self):
        v = ViaemsWrapper("obj/hosted/viaems")
        full = v.get(path=[])['response']

        def compare_subpaths(conn, master, current_path=[]):
            current = conn.get(path=current_path)['response']
            assert(master == current)
            if type(current) == dict:
                for k, v in current.items():
                    compare_subpaths(conn, v, current_path + [k])
            elif type(current) == list:
                for i, v in enumerate(current):
                    compare_subpaths(conn, v, current_path + [int(i)])

        compare_subpaths(v, full)

if __name__ == "__main__":
    unittest.main()
