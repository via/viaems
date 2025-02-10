import unittest
import sys

from viaems.connector import SimConnector, HilConnector


class TestCase(unittest.TestCase):

    def assertWithin(self, val, lower, upper, msg=None):
        self.assertGreaterEqual(val, lower, msg)
        self.assertLessEqual(val, upper, msg)

    def setUp(self):
        if "--hil" in sys.argv:
            self.conn = HilConnector()
        else:
            self.conn = SimConnector("obj/hosted/viaems")

    def tearDown(self):
        self.conn.kill()
