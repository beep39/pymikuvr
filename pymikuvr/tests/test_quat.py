import unittest

from api.vec3 import vec3
from api.quat import quat

class test_quat(unittest.TestCase):
    def assertAlmostEqual(self, a, b):
        if isinstance(a, quat):
            unittest.TestCase.assertAlmostEqual(self, a.x, b.x)
            unittest.TestCase.assertAlmostEqual(self, a.y, b.y)
            unittest.TestCase.assertAlmostEqual(self, a.z, b.z)
            unittest.TestCase.assertAlmostEqual(self, a.w, b.w)
        elif isinstance(a, vec3):
            unittest.TestCase.assertAlmostEqual(self, a.x, b.x)
            unittest.TestCase.assertAlmostEqual(self, a.y, b.y)
            unittest.TestCase.assertAlmostEqual(self, a.z, b.z)
        else:
            unittest.TestCase.assertAlmostEqual(self, a, b)

    def test_initialization_and_value_set(self):
        a = quat()
        self.assertEqual(a.x, 0)
        self.assertEqual(a.y, 0)
        self.assertEqual(a.z, 0)
        self.assertEqual(a.w, 1)

        b = quat.from_pyr(0,0,0)
        self.assertAlmostEqual(a, b)

        b.set(1,2,3,4)
        self.assertAlmostEqual(b, quat(1,2,3,4))

        c = quat(b)
        self.assertFalse(c is b)
        self.assertEqual(b, c)

        d = c.copy()
        self.assertFalse(c is d)
        self.assertEqual(c, d)

    def test_multiply(self):
        a = vec3(1,2,3)
        self.assertAlmostEqual(quat() * a, a)
        self.assertAlmostEqual(a * quat(), a)

        self.assertAlmostEqual(quat.from_pyr(90, 0, 0) * vec3.forward, vec3.up)
        self.assertAlmostEqual(vec3.up * quat.from_pyr(90, 0, 0), vec3.forward)

        b = quat.from_pyr(10, 20, 30)
        c = quat.from_pyr(-40, 50, -60)

        self.assertAlmostEqual(b * (c * a), (b * c) * a)
        self.assertAlmostEqual((a * b) * c, a * (b * c))

    def test_invert(self):
        a = quat.from_pyr(10, 20, 30)
        b = quat(a)

        #static
        quat.invert(a)
        self.assertEqual(a, b) #unchanged

        #instance
        self.assertAlmostEqual(a.invert(), quat.invert(b))
        self.assertAlmostEqual(a, quat.invert(b))

        self.assertAlmostEqual(quat.invert(quat()), quat())

        self.assertAlmostEqual(a * quat.invert(a), quat())
        self.assertAlmostEqual(a * (quat.invert(a) * b), b)

        c = vec3(1,2,3)
        self.assertAlmostEqual(quat.invert(a) * c, c * a)

    def test_rot_direction(self):

        # https://github.com/beep39/pymikuvr/wiki/Coordinate-system

        self.assertAlmostEqual(quat.from_pyr(90, 0, 0) * vec3(0,1,0), vec3(0,0,1))
        self.assertAlmostEqual(quat.from_pyr(0, 0, 90) * vec3(1,0,0), vec3(0,1,0))
        self.assertAlmostEqual(quat.from_pyr(0, 90, 0) * vec3(0,0,1), vec3(1,0,0))

        self.assertAlmostEqual(quat.from_pyr(90, 0, 0) * vec3.forward, vec3.up)
        self.assertAlmostEqual(quat.from_pyr(0, 0, 90) * vec3.right, vec3.up)
        self.assertAlmostEqual(quat.from_pyr(0, 90, 0) * vec3.right, vec3.forward)

    def test_distance(self):
        self.assertAlmostEqual(quat.distance(quat.from_pyr(90,0,0), quat.from_pyr(-90,0,0)), 1)
        self.assertAlmostEqual(quat.distance(quat.from_pyr(0,-180,0), quat.from_pyr(0,180,0)), 0)
        self.assertAlmostEqual(quat.distance(quat(), quat()), 0)
        self.assertAlmostEqual(quat.distance(quat(), quat(0,0,0,-1)), 0)

    def test_angles(self):
        q = quat.from_pyr(20, 40, 70)
        q2 = quat.from_pyr(0, 0, 70) * quat.from_pyr(0, 40, 0) * quat.from_pyr(20, 0, 0)
        self.assertAlmostEqual(q, q2)

        for a in [-90, -40, 0, 40, 90]:
            self.assertAlmostEqual(quat.from_pyr(a,0,0).pitch, a)
        for a in [-180, -90, -40, 0, 40, 90, 180]:
            self.assertAlmostEqual(quat.from_pyr(0,a,0).yaw, a)
        for a in [-90, -40, 0, 40, 90]:
            self.assertAlmostEqual(quat.from_pyr(0,0,a).roll, a)

        for p in [-90, -40, 0, 40, 90]:
            for y in [-180, -90, -40, 0, 40, 90, 180]:
                q = quat.from_pyr(p, y, 0)
                self.assertAlmostEqual(q.pitch, p)
                self.assertAlmostEqual(q.yaw, y)

                q2 = quat.from_pyr(q.pitch, q.yaw, q.roll)
                self.assertAlmostEqual(q, q2)
