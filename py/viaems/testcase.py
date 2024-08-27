import unittest

from viaems.connector import ViaemsWrapper


class TestCase(unittest.TestCase):

    def assertWithin(self, val, lower, upper):
        self.assertGreaterEqual(val, lower)
        self.assertLessEqual(val, upper)

    def setUp(self):
        self.conn = ViaemsWrapper("obj/hosted/viaems")

    def tearDown(self):
        self.conn.kill()
