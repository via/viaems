import unittest
import sys

from viaems.connector import SimConnector, HilConnector


class TestCase(unittest.TestCase):

    def assertWithin(self, val, lower, upper):
        self.assertGreaterEqual(val, lower)
        self.assertLessEqual(val, upper)

    def setUp(self):
        if "--hil" in sys.argv:
            self.conn = HilConnector()
        else:
            self.conn = SimConnector("obj/hosted/viaems")

        self.conn.start()

    def tearDown(self):
        self.conn.kill()
