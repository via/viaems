#!/usr/bin/env python3
import unittest
import sys

from viaems.testcase import TestCase

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


class ViaemsInterfaceTests(TestCase):

    def setUp(self):
        super().setUp()
        self.conn.start()

    def test_structure_full(self):
        result = self.conn.structure()
        assert(result['success'])
        assert("response" in result)

        assert(_leaves_have_types(result['response']))

        outputs = result['response']['outputs']
        assert(len(outputs) == 16)
        assert(outputs[0]['_type'] == 'output')

        sensors = result['response']['sensors']
        assert(len(sensors) == 12)
        assert(sensors['iat']['_type'] == 'sensor')

    def test_structure_types(self):
        result = self.conn.structure()
        assert(result['success'])
        assert("types" in result)

        sensor = result['types']['sensor']
        assert(_leaves_have_descriptions(sensor))
        assert(_leaves_have_types(sensor))
        output = result['types']['output']
        assert(_leaves_have_descriptions(output))
        assert(_leaves_have_types(output))

    def test_get_full(self):
        result = self.conn.get(path=[])
        assert(result['success'])

        outputs = result['response']['outputs']
        assert(type(outputs[1]['angle']) == float)
        assert(type(outputs[1]['pin']) == int)
        
        sensors = result['response']['sensors']
        assert(type(sensors['map']['pin']) == int)
        assert(type(sensors['map']['method']) == str)
        assert(type(sensors['map']['range-min']) == float)

    def test_get_all_subpaths(self):
        full = self.conn.get(path=[])['response']

        def compare_subpaths(master, current_path=[]):
            current = self.conn.get(path=current_path)['response']
            self.assertEqual(master, current)
            if type(current) == dict:
                for k, v in current.items():
                    compare_subpaths(v, current_path + [k])
            elif type(current) == list:
                for i, v in enumerate(current):
                    compare_subpaths(v, current_path + [int(i)])

        compare_subpaths(full)

    def test_set_map_floats(self):
        result = self.conn.set(path=[], value={"decoder": {"offset": 60.0}})
        assert(result['success'])
        assert(result['response']['decoder']['offset'] == 60.0)
        assert(self.conn.get(path=["decoder", "offset"])['response'] == 60.0)

    def test_set_map_float_via_path(self):
        result = self.conn.set(path=["decoder", "offset"], value=60.0)
        assert(result['success'])
        assert(result['response'] == 60.0)
        assert(self.conn.get(path=["decoder", "offset"])['response'] == 60.0)

    def test_set_map_multiple_floats(self):
        result = self.conn.set(path=[], value={
            "sensors": {
                "iat": {
                    "range-max": 25.0
                    },
                "map": {
                    "range-max": 26.0
                    }
                }
            })
        assert(result['success'])
        assert(result['response']['sensors']['iat']["range-max"] == 25.0)
        assert(result['response']['sensors']['map']["range-max"] == 26.0)
        assert(self.conn.get(path=["sensors", "iat", "range-max"])['response'] == 25.0)
        assert(self.conn.get(path=["sensors", "map", "range-max"])['response'] == 26.0)

    def test_set_array_float(self):
        result = self.conn.set(path=[], value={
            "outputs": [
                {
                    "pin": 10,
                },
                {
                    "pin": 11,
                }
            ]})
        assert(result['success'])
        assert(result['response']['outputs'][0]["pin"] == 10)
        assert(result['response']['outputs'][1]["pin"] == 11)

    def test_set_array_float_by_path(self):
        result = self.conn.set(path=["outputs", 1, "pin"], value=12)
        assert(result['success'])
        assert(result['response'] == 12)
    
    def test_set_sensor_map(self):
        result = self.conn.set(path=["sensors", "iat"], value={
            "method": "linear-window",
            "source": "const",
            "pin": 5,
            "range-min": 12.0,
            })
        assert(result['success'])
        assert(self.conn.get(path=["sensors", "iat", "range-min"])['response'] == 12.0)
        assert(self.conn.get(path=["sensors", "iat", "method"])['response'] == "linear-window")
        assert(self.conn.get(path=["sensors", "iat", "source"])['response'] == "const")
        assert(self.conn.get(path=["sensors", "iat", "pin"])['response'] == 5)

    def test_set_table_1d(self):
        result = self.conn.set(path=["tables", "injector_pw_correction"], value={
            "horizontal-axis": {
                "name": "bleh",
                "values": [0, 10, 20],
            },
            "data": [100, 200, 300],
            })
        assert(result['success'])
        assert(self.conn.get(path=["tables", "injector_pw_correction",
                                   "horizontal-axis", "name"])["response"] == "bleh")
        assert(self.conn.get(path=["tables", "injector_pw_correction",
                                   "horizontal-axis", "values"])["response"] == [0, 10, 20])
        assert(self.conn.get(path=["tables", "injector_pw_correction",
                                   "data"])["response"] == [100, 200, 300])

    def test_set_table_2d(self):
        result = self.conn.set(path=["tables", "ve"], value={
            "horizontal-axis": {
                "name": "bleh",
                "values": [0, 10, 20],
            },
            "vertical-axis": {
                "name": "bleh2",
                "values": [100, 200, 300.0],
            },
            "data": [
                [100, 200, 300],
                [400, 500, 600],
                [700, 800, 900],
                ]})
        assert(result['success'])
        assert(self.conn.get(path=["tables", "ve",
                                   "horizontal-axis", "name"])["response"] == "bleh")
        assert(self.conn.get(path=["tables", "ve",
                                   "horizontal-axis", "values"])["response"] == [0, 10, 20])
        assert(self.conn.get(path=["tables", "ve",
                                   "vertical-axis", "name"])["response"] == "bleh2")
        assert(self.conn.get(path=["tables", "ve",
                                   "vertical-axis", "values"])["response"] == [100, 200, 300])
        assert(self.conn.get(path=["tables", "ve",
                                   "data"])["response"] == [
                                       [100, 200, 300],
                                       [400, 500, 600],
                                       [700, 800, 900],
                                       ])



if __name__ == "__main__":
    if len(sys.argv) > 1:
        sys.argv = sys.argv[1:]
    unittest.main()
