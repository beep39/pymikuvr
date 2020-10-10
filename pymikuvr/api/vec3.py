import api.quat
import math

r2d = 180.0 / math.pi

class classproperty(property):
    def __get__(self, cls, owner):
        return classmethod(self.fget).__get__(None, owner)()

def unpack_vec3(init_f):
    def _wrapper(self, *args, **kws):
        if args and isinstance(args[0], vec3):
            return init_f(self, args[0].x, args[0].y, args[0].z)
        else:
            return init_f(self, *args, **kws)
    return _wrapper

class vec3:
    __slots__ = ('x', 'y', 'z')
    @unpack_vec3
    def __init__(self, x = 0.0, y = 0.0, z = 0.0):
        self.x = x
        self.y = y
        self.z = z

    def set(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        return self

    def copy(self):
        return vec3(self.x, self.y, self.z)

    def length(a):
        return math.sqrt(vec3.dot(a, a))

    @staticmethod
    def normalize(a):
        l = a.length()
        if l == 0.0:
            return vec3()
        return a / l

    def normalize(self):
        l = self.length()
        if l == 0.0:
            return self
        self.set(self.x / l, self.y / l, self.z / l)
        return self

    def dot(a, b):
        return a.x * b.x + a.y * b.y + a.z * b.z

    def cross(a, b):
        return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)

    def lerp(a, b, t):
        return a + (b - a) * t

    def tend(value, target, speed):
        diff = target - value
        dist = diff.length()
        if dist < speed:
            return target
        return value + diff * (speed / dist)

    @classproperty
    def forward(self):
        return vec3(0,0,-1)

    @classproperty
    def right(self):
        return vec3(1,0,0)

    @classproperty
    def up(self):
        return vec3(0,1,0)

    @property
    def yaw_rad(a):
        if a.x == 0.0 and a.z == 0.0:
            return 0.0
        return math.atan2(-a.x, -a.z)

    @property
    def yaw(a):
        return a.yaw_rad * r2d

    @property
    def pitch_rad(a):
        l = a.length()
        eps = 0.0001
        if l > eps:
            return -math.asin(a.y / l)
        if a.y > eps:
            return -math.pi * 0.5
        if a.y < -eps:
            return math.pi * 0.5
        return 0

    @property
    def pitch(a):
        return a.pitch_rad * r2d

    def __eq__(a, b):
        return a.x == b.x and a.y == b.y and a.z == b.z

    def __add__(a, b):
        return vec3(a.x + b.x, a.y + b.y, a.z + b.z)

    def __sub__(a, b):
        return vec3(a.x - b.x, a.y - b.y, a.z - b.z)

    def __mul__(a, b):
        if isinstance(b, vec3):
            return vec3(a.x * b.x, a.y * b.y, a.z * b.z)
        elif isinstance(b, api.quat.quat):
            t = vec3(b.x, b.y, b.z)
            return a + vec3.cross(vec3.cross(a, t) + a * b.w, t) * 2.0
        else:
            return vec3(a.x * b, a.y * b, a.z * b)

#    def __rmul__(a, b):
#        return a * b

    def __truediv__(a, b):
        if isinstance(b, vec3):
            return vec3(a.x / b.x, a.y / b.y, a.z / b.z)
        else:
            bb = 1.0 / b
            return vec3(a.x * bb, a.y * bb, a.z * bb)

    def __neg__(a):
        return vec3(-a.x, -a.y, -a.z)

    def __str__(self):
        return 'vec3({:.2f},{:.2f},{:.2f})'.format(self.x, self.y, self.z)

class vec3_c(vec3):
    __slots__ = ('__x', '__y', '__z')
    @unpack_vec3
    def __init__(self, x = 0.0, y = 0.0, z = 0.0):
        self.__x = x
        self.__y = y
        self.__z = z

    def set(self, x, y, z):
        raise AttributeError('class is constant')

    @property
    def x(self):
        return self.__x

    @x.setter
    def x(self, x):
        raise AttributeError('class is constant')

    @property
    def y(self):
        return self.__y

    @y.setter
    def y(self, y):
        raise AttributeError('class is constant')

    @property
    def z(self):
        return self.__z

    @z.setter
    def z(self, z):
        raise AttributeError('class is constant')

class vec3_o(vec3):
    __slots__ = ('__observer_callback', '__x', '__y', '__z')
    def __init__(self, observer_callback, x = 0.0, y = 0.0, z = 0.0):
        self.__x = x
        self.__y = y
        self.__z = z
        self.__observer_callback = observer_callback

    def set(self, x, y, z):
        self.__x = x
        self.__y = y
        self.__z = z
        self.__observer_callback()
        return self

    @property
    def x(self):
        return self.__x

    @x.setter
    def x(self, x):
        self.__x = x
        self.__observer_callback()

    @property
    def y(self):
        return self.__y

    @y.setter
    def y(self, y):
        self.__y = y
        self.__observer_callback()

    @property
    def z(self):
        return self.__z

    @z.setter
    def z(self, z):
        self.__z = z
        self.__observer_callback()
