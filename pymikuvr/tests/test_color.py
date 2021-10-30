import unittest

from api.color import color, color_o

class test_color(unittest.TestCase):
    def assertAlmostEqual(self, a, b):
        if isinstance(a, color):
            unittest.TestCase.assertAlmostEqual(self, a.r, b.r)
            unittest.TestCase.assertAlmostEqual(self, a.g, b.g)
            unittest.TestCase.assertAlmostEqual(self, a.b, b.b)
            unittest.TestCase.assertAlmostEqual(self, a.a, b.a)
        else:
            unittest.TestCase.assertAlmostEqual(self, a, b)

    def test_eq(self):
        a = color(1,2,3,4)
        b = color(1,2,3,4)
        c = color()
        self.assertTrue(a==b)
        self.assertFalse(b==c)

    def test_arifmetic(self):
        a = color(0.1,0.2,0.3,0.4)
        a2 = color(a.r * 2, a.g * 2, a.b * 2, a.a * 2)
        b = color(0.5,0.6,0.7,0.8)
        zero = color(0,0,0,0)
        self.assertAlmostEqual(a + b - a, b)
        #self.assertAlmostEqual(a * b / a, b)
        #self.assertAlmostEqual(a / 1, a)
        #self.assertAlmostEqual(a / 2, color(a.r / 2, a.g / 2, a.b / 2, a.a / 2))
        self.assertAlmostEqual(a * 1, a)
        self.assertAlmostEqual(a * 0, zero)
        self.assertAlmostEqual(a * 2, a2)
        self.assertAlmostEqual(0 * a, zero)
        self.assertEqual(1 * a, a)
        self.assertEqual(2 * a, a2)
