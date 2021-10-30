import api.vec3
import math
from collections import namedtuple

r2d = 180.0 / math.pi
d2r = math.pi / 180.0
pyr = namedtuple('pyr', 'pitch yaw roll')

def unpack_quat(init_f):
    def _wrapper(self, *args, **kws):
        if args and isinstance(args[0], quat):
            return init_f(self, args[0].x, args[0].y, args[0].z, args[0].w)
        else:
            return init_f(self, *args, **kws)
    return _wrapper

class static_or_instance(property):
    def __get__(self, instance, owner):
        if instance is None:
            return self.fget
        def result():
            a = self.fget(instance)
            instance.set(a.x,a.y,a.z,a.w)
            return instance
        return result

class quat:
    __slots__ = ('x', 'y', 'z', 'w', '__observer_callback')
    @unpack_quat
    def __init__(self, x = 0.0, y = 0.0, z = 0.0, w = 1.0):
        self.x = x
        self.y = y
        self.z = z
        self.w = w

    @staticmethod
    def from_pyr(pitch, yaw, roll):
        return quat().set_pyr(pitch, yaw, roll)

    def set(self, x, y, z, w):
        self.x = x
        self.y = y
        self.z = z
        self.w = w
        return self

    def copy(self):
        return quat(self.x, self.y, self.z, self.w)

    def set_pyr_rad(self, pitch, yaw, roll):
        hp = pitch * 0.5
        hy = yaw * 0.5
        hr = roll * 0.5

        sx = math.sin(hp)
        cx = math.cos(hp)

        sy = math.sin(hy)
        cy = math.cos(hy)

        sz = math.sin(hr)
        cz = math.cos(hr)

        x = sx * cy * cz - cx * sy * sz
        y = cx * sy * cz + sx * cy * sz
        z = cx * cy * sz - sx * sy * cz
        w = cx * cy * cz + sx * sy * sz
        self.set(x, y, z, w)
        return self

    def set_pyr(self, pitch, yaw, roll):
        return self.set_pyr_rad(pitch * d2r, yaw * d2r, roll * d2r)

    def get_pyr_rad(self):
        yz = self.y * self.z
        wx = self.w * self.x
        yy = self.y * self.y
        xy = self.x * self.y
        zz = self.z * self.z
        wz = self.w * self.z

        temp = wx - yz
        temp = temp + temp
        if temp >= 1.0:
            return pyr(math.pi * 0.5, math.atan2(xy - wz, 0.5 - yy - zz), 0)
        if temp <= -1.0:
            return pyr(-math.pi * 0.5, -math.atan2(xy - wz, 0.5 - yy - zz), 0)

        xz = self.x * self.z
        wy = self.w * self.y
        xx = self.x * self.x

        pitch = math.asin(temp)
        yaw = math.atan2(xz + wy, 0.5 - yy - xx)
        roll = math.atan2(xy + wz, 0.5 - xx - zz)
        return pyr(pitch, yaw, roll)

    def get_pyr(self):
        t = self.get_pyr_rad()
        return pyr(t.pitch * r2d, t.yaw * r2d, t.roll * r2d)

    @property
    def pitch(self):
        temp = self.w * self.x - self.y * self.z
        temp = temp + temp
        if temp >= 1.0:
            return 90
        elif temp <= -1.0:
            return -90
        return math.asin(temp) * r2d

    @pitch.setter
    def pitch(self, v):
        pyr = self.get_pyr()
        self.set_pyr(v, pyr.yaw, pyr.roll)

    @property
    def yaw(self):
        return self.get_pyr().yaw

    @yaw.setter
    def yaw(self, v):
        pyr = self.get_pyr()
        self.set_pyr(pyr.pitch, v, pyr.roll)

    @property
    def roll(self):
        tmp = self.x * self.y + self.w * self.z
        if abs(tmp) < 1e-10:
            return 0
        return math.atan2(tmp, 0.5 - self.x * self.x - self.z * self.z) * r2d

    @roll.setter
    def roll(self, v):
        pyr = self.get_pyr()
        self.set_pyr(pyr.pitch, pyr.yaw, v)

    @static_or_instance
    def invert(v):
        return quat(-v.x, -v.y, -v.z, v.w)

    @staticmethod
    def distance(q1, q2):
        cosom = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w
        return 1.0 - abs(cosom)

    @staticmethod
    def angle_rad(q1, q2):
        cosom = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w
        if cosom == 0.0:
            return 0.0
        return math.acos(min(abs(cosom), 1.0)) * 2.0

    @staticmethod
    def angle(q1, q2):
        return quat.angle_rad(q1, q2) * r2d

    @staticmethod
    def slerp(q1, q2, t):
        eps = 0.001
        scale0 = 0
        scale1 = 0

        cosom = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w
        if cosom < 0.0:
            if 1.0 + cosom > eps:
                omega = math.acos(-cosom)
                sinom_inv = 1.0 / math.sin(omega)
                scale0 = math.sin((1.0 - t) * omega) * sinom_inv
                scale1 = -math.sin(t * omega) * sinom_inv
            else:
                scale0 = 1.0 - t
                scale1 = -t
        else:
            if 1.0 - cosom > eps:
                omega = math.acos(cosom)
                sinom_inv = 1.0 / math.sin(omega)
                scale0 = math.sin((1.0 - t) * omega) * sinom_inv
                scale1 = math.sin(t * omega) * sinom_inv
            else:
                scale0 = 1.0 - t
                scale1 = t

        x = scale0 * q1.x + scale1 * q2.x
        y = scale0 * q1.y + scale1 * q2.y
        z = scale0 * q1.z + scale1 * q2.z
        w = scale0 * q1.w + scale1 * q2.w
        return quat(x, y, z, w)

    @staticmethod
    def tend(value, target, speed):
        angle = quat.angle(value, target)
        if angle < speed:
            return target
        return quat.slerp(value, target, speed / angle)

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y and self.z == other.z and self.w == other.w

    def __mul__(self, other):
        if isinstance(other, quat):
            x = self.w * other.x + self.x * other.w + self.y * other.z - self.z * other.y
            y = self.w * other.y - self.x * other.z + self.y * other.w + self.z * other.x
            z = self.w * other.z + self.x * other.y - self.y * other.x + self.z * other.w
            w = self.w * other.w - self.x * other.x - self.y * other.y - self.z * other.z
            return quat(x, y, z, w)

        if isinstance(other, api.vec3.vec3):
            vec3 = api.vec3.vec3
            t = vec3(self.x, self.y, self.z)
            return other + vec3.cross(t, vec3.cross(t, other) + other * self.w) * 2.0

        raise ValueError("unable to multiply quat by " + type(other))

    @static_or_instance
    def normalize(a):
        l = math.sqrt(a.x * a.x + a.y * a.y + a.z + a.z + a.w * a.w)
        if l == 0.0:
            return quat()
        l = 1 / l
        return quat(a.x * l, a.y * l, a.z * l, a.w * l)

    def __str__(self):
        return 'quat({:.2f},{:.2f},{:.2f},{:.2f})'.format(self.x, self.y, self.z, self.w)

class quat_c(quat):
    __slots__ = ('__x', '__y', '__z', '__w')
    @unpack_quat
    def __init__(self, x = 0.0, y = 0.0, z = 0.0, w = 1.0):
        self.__x = x
        self.__y = y
        self.__z = z
        self.__w = w

    def set(self, x, y, z, w):
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

    @property
    def w(self):
        return self.__w

    @w.setter
    def w(self, w):
        raise AttributeError('class is constant')

class quat_o(quat):
    __slots__ = ('__observer_callback', '__x', '__y', '__z', '__w')
    @unpack_quat
    def __init__(self, observer_callback, x = 0.0, y = 0.0, z = 0.0, w = 1.0):
        self.__x = x
        self.__y = y
        self.__z = z
        self.__w = w
        self.__observer_callback = observer_callback

    def set(self, x, y, z, w):
        self.__x = x
        self.__y = y
        self.__z = z
        self.__w = w
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

    @property
    def w(self):
        return self.__w
    
    @w.setter
    def w(self, w):
        self.w = __w
        self.__observer_callback()
