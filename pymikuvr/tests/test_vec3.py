import unittest

from api.vec3 import vec3

class test_vec3(unittest.TestCase):
    def assertAlmostEqual(self, a, b):
        if isinstance(a, vec3):
            unittest.TestCase.assertAlmostEqual(self, a.x, b.x)
            unittest.TestCase.assertAlmostEqual(self, a.y, b.y)
            unittest.TestCase.assertAlmostEqual(self, a.z, b.z)
        else:
            unittest.TestCase.assertAlmostEqual(self, a, b)

    def test_initialization_and_value_set(self):
        a = vec3()
        self.assertEqual(a.x, 0)
        self.assertEqual(a.y, 0)
        self.assertEqual(a.z, 0)

        b = vec3(1,2,3)
        self.assertEqual(b.x, 1)
        self.assertEqual(b.y, 2)
        self.assertEqual(b.z, 3)

        c = vec3(b)
        self.assertFalse(c is b)
        self.assertEqual(b, c)

        d = vec3(b)
        self.assertFalse(d is b)
        self.assertEqual(d, b)

        e = vec3()
        e.set(1,2,3)
        self.assertEqual(e, b)

        f = e.copy()
        self.assertFalse(f is e)
        self.assertEqual(f, e)

    def test_eq(self):
        a = vec3(1,2,3)
        b = vec3(1,2,3)
        c = vec3()
        self.assertTrue(a==b)
        self.assertFalse(b==c)

    def test_length_and_normalize(self):
        a = vec3(1,2,3)
        b = vec3(1,2,3)
        c = vec3()
        d = vec3(3,2,6)

        self.assertAlmostEqual(c.length(), 0)
        self.assertAlmostEqual(d.length(), 7)

        #static
        self.assertAlmostEqual(vec3.normalize(a).length(), 1)
        self.assertEqual(a, b) #unchanged

        #instance
        self.assertAlmostEqual(a.normalize().length(), 1)
        self.assertAlmostEqual(a.length(), 1) #normalized

        #zero length
        self.assertAlmostEqual(c.normalize().length(), 0)

    def test_cross_order(self):

        # right-handed coordinate system
        
        a = vec3(1,0,0)
        b = vec3(0,1,0)
        c = vec3(0,0,1)
        self.assertAlmostEqual(vec3.cross(a, b), c) 
        self.assertAlmostEqual(vec3.cross(c, a), b) 
        self.assertAlmostEqual(vec3.cross(b, c), a) 
        self.assertAlmostEqual(vec3.cross(vec3.right, vec3.forward), vec3.up)

    def test_arifmetic(self):
        a = vec3(1,2,3)
        a2 = vec3(1*2,2*2,3*2)
        b = vec3(-4,5,-6)
        self.assertAlmostEqual(a + b - a, b)
        self.assertAlmostEqual(-a + b + a, b)
        self.assertAlmostEqual(a * b / a, b)
        self.assertAlmostEqual(a / 1, a)
        self.assertAlmostEqual(a / 2, vec3(a.x / 2, a.y / 2, a.z / 2))
        self.assertAlmostEqual(a * 1, a)
        self.assertAlmostEqual(a * 0, vec3())
        self.assertAlmostEqual(a * 2, a2)
        self.assertAlmostEqual(0 * a, vec3())
        self.assertEqual(1 * a, a)
        self.assertEqual(2 * a, a2)

    def test_lerp(self):
        a = vec3(1,2,3)
        b = vec3(-4,5,-6)
        self.assertAlmostEqual(vec3.lerp(a, b, 0), a)
        self.assertAlmostEqual(vec3.lerp(a, b, 1), b)
        self.assertAlmostEqual(vec3.lerp(a, b, 0.5), (a + b) / 2)

    def test_tend(self):
        a = vec3(-1,2,3)
        diff = vec3(3,6,2) # length is 12
        b = a + diff
        self.assertAlmostEqual(vec3.tend(a, b, 0), a)
        self.assertAlmostEqual(vec3.tend(a, b, diff.length()), b)
        self.assertAlmostEqual(vec3.tend(a, b, diff.length() * 0.5), (a + b) / 2)
        self.assertAlmostEqual(vec3.tend(a, a, 0), a)
        self.assertAlmostEqual(vec3.tend(a, a, 100), a)

    def test_angles(self):

        # https://github.com/beep39/pymikuvr/wiki/Coordinate-system

        self.assertAlmostEqual(vec3.right.pitch, 0)
        self.assertAlmostEqual(vec3.up.pitch, 90)
        self.assertAlmostEqual(vec3.forward.pitch, 0)

        self.assertAlmostEqual(vec3.right.yaw, -90)
        self.assertAlmostEqual(vec3.up.yaw, 0)
        self.assertAlmostEqual(vec3.forward.yaw, 0)
