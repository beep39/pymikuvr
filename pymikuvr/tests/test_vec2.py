import unittest

from api.vec2 import vec2

def compare(a, b):
    return abs(a-b) < 0.00001

class test_vec2(unittest.TestCase):
    def assertAlmostEqual(self, a, b):
        if isinstance(a, vec2):
            unittest.TestCase.assertAlmostEqual(self, a.x, b.x)
            unittest.TestCase.assertAlmostEqual(self, a.y, b.y)
        else:
            unittest.TestCase.assertAlmostEqual(self, a, b)

    def test_eq(self):
        a = vec2(1,2)
        b = vec2(1,2)
        c = vec2()
        self.assertTrue(a==b)
        self.assertFalse(b==c)

    def test_normalize(self):
        a = vec2(1,2)
        b = vec2(1,2)
        c = vec2()

        initial_length = a.length()

        #static
        self.assertAlmostEqual(vec2.normalize(a).length(), 1)
        self.assertEqual(a, b)

        #instance
        self.assertAlmostEqual(a.normalize().length(), 1)
        self.assertAlmostEqual(a.length(), 1)

        #zero length
        self.assertAlmostEqual(c.normalize().length(), 0)
