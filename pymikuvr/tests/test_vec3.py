import unittest

from api.vec3 import vec3

class test_vec3(unittest.TestCase):
    def assertAlmostEqual(self, a, b):
        return self.assertTrue(abs(a-b) < 0.00001)

    def test_eq(self):
        a = vec3(1,2,3)
        b = vec3(1,2,3)
        c = vec3()
        self.assertTrue(a==b)
        self.assertFalse(b==c)

    def test_normalize(self):
        a = vec3(1,2,3)
        b = vec3(1,2,3)
        c = vec3()

        #static
        self.assertAlmostEqual(vec3.normalize(a).length(), 1)
        self.assertEqual(a, b) #unchanged

        #instance
        self.assertAlmostEqual(a.normalize().length(), 1)
        self.assertAlmostEqual(a.length(), 1) #normalized

        #zero length
        self.assertAlmostEqual(c.normalize().length(), 0)
