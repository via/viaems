#!/usr/bin/env python3
from enum import Enum
import unittest
import struct
import sys
import time

from viaems.testcase import TestCase
from viaems_proto import console_pb2 as console


class ViaemsInterfaceTests(TestCase):

    def setUp(self):
        super().setUp()
        self.conn.start()
        self.default_config = self.conn.getconfig()

#    def mutate(self, element):
#        if element == None:
#            return None
#        elif isinstance(element, bool):
#            return not element
#        elif isinstance(element, Enum):
#            return element # TODO choose another enum
#        elif isinstance(element, int):
#            return element + 1
#        elif isinstance(element, float):
#            return element + 0.1
#        elif isinstance(element, list):
#            result = element
#            element[0] = self.mutate(element[0])
#            return element
#        elif isinstance(element, str):
#            return "Different"
#        elif isinstance(element, bytes):
#            return b"Different"
#        elif isinstance(element, object):
#            for (desc, val) in  element.ListKeys():
#                setattr(element, key, self.mutate(getattr(element, key)))
#            return element

#    def test_mutate_every_option(self):
#        """Issues commands to change every field and validate that the change is accepted.
#           Note, this test can be brittle, as it assumes the default config can 
#           tolerate the slight changes we make to all fields
#           """
#
#        mutated = self.mutate(self.default_config)
#
#        # To force doubles to 32 bit floats
#        roundtrip = console.Configuration.parse(bytes(mutated))
#
#        self.conn.setconfig(mutated)
#        newconfig = self.conn.getconfig().response.getconfig.config
#
#        assert newconfig.to_dict() == roundtrip.to_dict()


    def test_set_sensor_map(self):
        config = console.Configuration()
        config.sensors.iat.method = console.Configuration.SensorMethod.METHOD_LINEAR_WINDOWED
        config.sensors.iat.source = console.Configuration.SensorSource.SOURCE_CONST
        config.sensors.iat.pin = 5
        config.sensors.iat.linear_config.input_min = 12

        result = self.conn.setconfig(config)

        assert result.response.setconfig.success == True

        config = result.response.setconfig.config
        assert(config.sensors.iat.method == console.Configuration.SensorMethod.METHOD_LINEAR_WINDOWED)
        assert(config.sensors.iat.source == console.Configuration.SensorSource.SOURCE_CONST)
        assert(config.sensors.iat.pin == 5)
        assert(config.sensors.iat.linear_config.input_min == 12)

    def test_set_table_1d(self):
        conf = console.Configuration()
        conf.fueling.injector_dead_time.cols.name = "bleh"
        conf.fueling.injector_dead_time.cols.values.extend([0, 10, 20])
        conf.fueling.injector_dead_time.data.values.extend([100, 200, 300])

        result = self.conn.setconfig(conf)

        assert result.response.setconfig.success == True
        conf = result.response.setconfig.config
        assert conf.fueling.injector_dead_time.cols.name == "bleh"
        assert conf.fueling.injector_dead_time.cols.values == [0, 10, 20]
        assert conf.fueling.injector_dead_time.data.values == [100, 200, 300]

    def test_set_table_2d(self):

        conf = console.Configuration()
        conf.fueling.ve.cols.name = "bleh"
        conf.fueling.ve.cols.values.extend([0, 10, 20])

        conf.fueling.ve.rows.name = "bleh2"
        conf.fueling.ve.rows.values.extend([100, 200, 300])

        conf.fueling.ve.data.extend([
            console.Configuration.TableRow(values=[100, 200, 300]),
            console.Configuration.TableRow(values=[400, 500, 600]),
            console.Configuration.TableRow(values=[700, 800, 900])
            ])

        result = self.conn.setconfig(conf)
        assert result.response.setconfig.success == True
        conf = result.response.setconfig.config
        assert conf.fueling.ve.cols.name == "bleh"
        assert conf.fueling.ve.cols.values == [0, 10, 20]
        assert conf.fueling.ve.rows.name == "bleh2"
        assert conf.fueling.ve.rows.values == [100, 200, 300]

        assert len(conf.fueling.ve.data) == 3
        assert conf.fueling.ve.data[0].values == [100, 200, 300]
        assert conf.fueling.ve.data[1].values == [400, 500, 600]
        assert conf.fueling.ve.data[2].values == [700, 800, 900]


class ConsoleUsageWhileRunning(TestCase):

    def test_set_table(self):
        self.conn.start()

        testtrigger_message = console.Message()
        testtrigger_message.request.setdebug.test_trigger_rpm = 5000
        self.conn.send(testtrigger_message)
        assert self.conn.recv_response().response.setdebug.test_trigger_rpm == 5000

        original = self.conn.getconfig().fueling.ve
        original.data[2].values[3] = 55.0

        for attempt in range(10):
            self.conn.sleep(1)
            config = console.Configuration()
            config.fueling.ve.CopyFrom(original)
            response = self.conn.setconfig(config)
            
            after = response.response.setconfig.config.fueling.ve
            assert after == original


if __name__ == "__main__":
    if len(sys.argv) > 1:
        sys.argv = sys.argv[1:]
    unittest.main()
