import unittest
import gc

from api.transform import transform
from api.quat import quat
from api.vec3 import vec3

class test_transform(unittest.TestCase):
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

    def test_posrot(self):
        v = vec3(1,2,3)
        q = quat.from_pyr(10, 20, 30)

        a = transform()
        a.pos.set(v.x, v.y, v.z)
        self.assertAlmostEqual(a.pos, v)
        self.assertAlmostEqual(a.local_pos, v)
        a.rot.set(q.x, q.y, q.z, q.w)
        self.assertAlmostEqual(a.rot, q)
        self.assertAlmostEqual(a.local_rot, q)

        b = transform()
        b.pos = v
        self.assertAlmostEqual(b.pos, v)
        self.assertAlmostEqual(b.local_pos, v)
        b.rot = q
        self.assertAlmostEqual(b.rot, q)
        self.assertAlmostEqual(b.local_rot, q)

        c = transform()
        c.local_pos.set(v.x, v.y, v.z)
        self.assertAlmostEqual(c.pos, v)
        self.assertAlmostEqual(c.local_pos, v)
        c.local_rot.set(q.x, q.y, q.z, q.w)
        self.assertAlmostEqual(c.rot, q)
        self.assertAlmostEqual(c.local_rot, q)

        d = transform()
        d.local_pos = v
        self.assertAlmostEqual(d.pos, v)
        self.assertAlmostEqual(d.local_pos, v)
        d.local_rot = q
        self.assertAlmostEqual(d.rot, q)
        self.assertAlmostEqual(d.local_rot, q)

        gc.collect()

    def test_hierarchy(self):
        v = vec3(1,2,3)
        v2 = vec3(4,5,6)

        a = transform()
        a.parent = None
        with self.assertRaises(ValueError) as context:
            a.parent = a
        self.assertEqual(a.parent, None)
        self.assertEqual(len(a.children), 0)

        b = transform()
        b.pos = v
        c = transform()
        c.pos = v2
        self.assertAlmostEqual(b.local_pos, v)
        c.parent = b
        self.assertEqual(c.parent, b)
        self.assertEqual(len(b.children), 1)
        self.assertEqual(b.children[0], c)
        self.assertAlmostEqual(b.local_pos, v)
        self.assertAlmostEqual(b.pos, v)
        self.assertAlmostEqual(c.pos, v2)
        c.parent = None
        self.assertEqual(c.parent, None)
        self.assertEqual(len(b.children), 0)

        c.parent = b
        c.parent = b
        with self.assertRaises(ValueError) as context:
            b.parent = c
        self.assertEqual(b.parent, None)
        self.assertEqual(c.parent, b)
        self.assertEqual(len(b.children), 1)
        self.assertEqual(b.children[0], c)

        c.parent = None
        self.assertEqual(c.parent, None)
        self.assertEqual(len(b.children), 0)

        d = transform()
        d.pos = v
        e = transform()
        e.pos = v2
        e.add_child(d)
        self.assertEqual(d.parent, e)
        self.assertAlmostEqual(d.local_pos, vec3())
        self.assertAlmostEqual(d.pos, e.pos)

        d.set_parent(None)
        self.assertEqual(d.parent, None)
        self.assertAlmostEqual(d.pos, e.pos)

        d.set_parent(e)
        self.assertEqual(d.parent, e)
        self.assertAlmostEqual(d.local_pos, vec3())
        self.assertAlmostEqual(d.pos, e.pos)

        f = transform()
        f.pos = v
        g = transform()
        g.pos = v2
        g.set_parent(f, False)
        self.assertAlmostEqual(g.local_pos, v2 - v)
        self.assertAlmostEqual(f.pos, v)

        gc.collect()
