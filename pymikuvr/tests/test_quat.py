import unittest

from api.vec3 import vec3
from api.quat import quat

class test_quat(unittest.TestCase):
    def assertAlmostEqual(self, a, b):
        if isinstance(a, vec3):
            unittest.TestCase.assertAlmostEqual(self, a.x, b.x)
            unittest.TestCase.assertAlmostEqual(self, a.y, b.y)
            unittest.TestCase.assertAlmostEqual(self, a.z, b.z)
        else:
            unittest.TestCase.assertAlmostEqual(self, a, b)

    def test_rot_direction(self):

        # https://github.com/beep39/pymikuvr/wiki/Coordinate-system

        self.assertAlmostEqual(quat.from_pyr(90, 0, 0) * vec3(0,1,0), vec3(0,0,1))
        self.assertAlmostEqual(quat.from_pyr(0, 0, 90) * vec3(1,0,0), vec3(0,1,0))
        self.assertAlmostEqual(quat.from_pyr(0, 90, 0) * vec3(0,0,1), vec3(1,0,0))

        self.assertAlmostEqual(quat.from_pyr(90, 0, 0) * vec3.forward, vec3.up)
        self.assertAlmostEqual(quat.from_pyr(0, 0, 90) * vec3.right, vec3.up)
        self.assertAlmostEqual(quat.from_pyr(0, 90, 0) * vec3.right, vec3.forward)
