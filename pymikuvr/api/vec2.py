import math

r2d = 180.0 / math.pi
d2r = math.pi / 180.0

class static_or_instance(property):
    def __get__(self, instance, owner):
        if instance is None:
            return self.fget
        def result():
            a = self.fget(instance)
            instance.set(a.x,a.y)
            return instance
        return result

def unpack_vec2(init_f):
    def _wrapper(self, *args, **kws):
        if args and isinstance(args[0], vec2):
            return init_f(self, args[0].x, args[0].y)
        else:
            return init_f(self, *args, **kws)
    return _wrapper

class vec2:
    __slots__ = ('x', 'y')
    @unpack_vec2
    def __init__(self, x = 0.0, y = 0.0):
        self.x = x
        self.y = y

    def set(self, x, y):
        self.x = x
        self.y = y
        return self

    def length(a):
        l = math.sqrt(vec2.dot(a, a))
        return l

    @static_or_instance
    def normalize(a):
        l = a.length()
        if l == 0.0:
            return vec2()
        return a / l

    def dot(a, b):
        return a.x * b.x + a.y * b.y

    def lerp(a, b, t):
        return a + (b - a) * t

    @staticmethod
    def rotate(v, a):
        a *= d2r
        c = math.cos(a)
        s = math.sin(a)
        return vec2(c * v.x + s * v.y, c * v.y - s * v.x)

    def rotated(self, a):
        a *= d2r
        c = math.cos(a)
        s = math.sin(a)
        return vec2(c * self.x + s * self.y, c * self.y - s * self.x)

    @staticmethod
    def yaw_rad(a):
        if a.x == 0.0 and a.y == 0.0:
            return 0.0
        return math.atan2(-a.x, -a.y)

    @staticmethod
    def yaw(a):
        return vec2.yaw_rad(a) * r2d

    @property
    def yaw(self):
        return vec2.yaw_rad(self) * r2d

    def __eq__(a, b):
        return a.x == b.x and a.y == b.y

    def __add__(a, b):
        v = vec2()
        v.x = a.x + b.x
        v.y = a.y + b.y
        return v

    def __sub__(a, b):
        v = vec2()
        v.x = a.x - b.x
        v.y = a.y - b.y
        return v

    def __mul__(a, b):
        v = vec2()
        if isinstance(b, vec2):
            v.x = a.x * b.x
            v.y = a.y * b.y
        else:
            v.x = a.x * b
            v.y = a.y * b
        return v

    def __truediv__(a, b):
        v = vec2()
        v.x = a.x / b
        v.y = a.y / b
        return v

    def __rmul__(a, b):
        return a * b

    def __neg__(a):
        v = vec3()
        v.x = -a.x
        v.y = -a.y
        return v

    def __str__(self):
        return 'vec2({:.2f},{:.2f})'.format(self.x, self.y)
