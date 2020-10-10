import math

r2d = 180.0 / math.pi
d2r = math.pi / 180.0

def unpack_vec2(init_f):
    def _wrapper(self, *args, **kws):
        if args and isinstance(args[0], vec2):
            return init_f(self, args[0].x, args[0].y)
        else:
            return init_f(self, *args, **kws)
    return _wrapper

class vec2:
    __slots__ = ('__x', '__y', '__observer_callback')
    @unpack_vec2
    def __init__(self, x = 0.0, y = 0.0, observer_callback = None):
        self.__x = x
        self.__y = y
        self.__observer_callback = observer_callback

    def set(self, x, y, z):
        self.__x = x
        self.__y = y
        if self.__observer_callback:
            self.__observer_callback()
        return self

    def length(a):
        return math.sqrt(vec2.dot(a, a))

    @staticmethod
    def normalize(a):
        l = a.length()
        if l == 0.0:
            return vec2()
        return a * (1.0 / l)

    def normalize(self):
        l = self.length()
        if l == 0.0:
            return self
        self.__x /= l
        self.__y /= l
        if self.__observer_callback:
            self.__observer_callback()
        return self

    def dot(a, b):
        return a.__x * b.__x + a.__y * b.__y

    def lerp(a, b, t):
        return a + (b - a) * t

    @staticmethod
    def rotate(v, a):
        a *= d2r
        c = math.cos(a)
        s = math.sin(a)
        return vec2(c * v.__x + s * v.__y, c * v.__y - s * v.__x)

    def rotated(self, a):
        a *= d2r
        c = math.cos(a)
        s = math.sin(a)
        return vec2(c * self.__x + s * self.__y, c * self.__y - s * self.__x)

    @staticmethod
    def yaw_rad(a):
        if a.__x == 0.0 and a.__y == 0.0:
            return 0.0
        return math.atan2(-a.__x, -a.__y)

    @staticmethod
    def yaw(a):
        return vec2.yaw_rad(a) * r2d

    @property
    def yaw(self):
        return vec2.yaw_rad(self) * r2d

    @property
    def x(self):
        return self.__x

    @x.setter
    def x(self, x):
        self.__x = x
        if self.__observer_callback:
            self.__observer_callback()

    @property
    def y(self):
        return self.__y

    @y.setter
    def y(self, y):
        self.__y = y
        if self.__observer_callback:
            self.__observer_callback()

    def __eq__(a, b):
        return a.__x == b.__x and a.__y == b.__y

    def __add__(a, b):
        v = vec2()
        v.__x = a.__x + b.__x
        v.__y = a.__y + b.__y
        return v

    def __sub__(a, b):
        v = vec2()
        v.__x = a.__x - b.__x
        v.__y = a.__y - b.__y
        return v

    def __mul__(a, b):
        v = vec2()
        if isinstance(b, vec2):
            v.__x = a.__x * b.__x
            v.__y = a.__y * b.__y
        else:
            v.__x = a.__x * b
            v.__y = a.__y * b
        return v

    def __truediv__(a, b):
        v = vec2()
        v.__x = a.__x / b
        v.__y = a.__y / b
        return v

    def __rmul__(a, b):
        return a * b

    def __neg__(a):
        v = vec3()
        v.__x = -a.__x
        v.__y = -a.__y
        return v

    def __str__(self):
        return 'vec2({:.2f},{:.2f})'.format(self.__x, self.__y)
