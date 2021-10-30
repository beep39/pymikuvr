from collections.abc import Sequence

class classproperty(property):
    def __get__(self, cls, owner):
        return classmethod(self.fget).__get__(None, owner)()

def unpack_color(init_f):
    def _wrapper(self, *args, **kws):
        if not args:
            return init_f(self)
        c = args[0]
        if isinstance(c, color):
            return init_f(self, c.r, c.g, c.b, c.a)
        elif isinstance(c, str):
            c = c.lstrip('#')
            l = len(c)
            return _wrapper(self, tuple(int(c[i:i + 2], 16) / 0xff for i in range(0, l, l // 3)))
        elif isinstance(c, Sequence):
            l = len(c)
            if l == 3:
                return init_f(self, c[0], c[1], c[2])
            elif l >= 4:
                return init_f(self, c[0], c[1], c[2], c[3])
            else:
                raise ValueError("insufficient sequence length ({}) to initialize color".format(l))
        return init_f(self, *args, **kws)
    return _wrapper

class color:
    @unpack_color
    def __init__(self, r = 1, g = 1, b = 1, a = 1):
        self.r = r
        self.g = g
        self.b = b
        self.a = a

class color:
    __slots__ = ('r', 'g', 'b', 'a')
    @unpack_color
    def __init__(self, r = 1, g = 1, b = 1, a = 1):
        self.r = r
        self.g = g
        self.b = b
        self.a = a

    @unpack_color
    def set(self, r, g, b, a = 1):
        self.r = r
        self.g = g
        self.b = b
        self.a = a
        return self

    @property
    def hex(self):
        return '#%02x%02x%02x%02x' % tuple(int(i * 0xff) for i in (self.r, self.g, self.b, self.a))

    @hex.setter
    def hex(self, value):
        self.__init__(value)

    def __eq__(self, other):
        return self.r == other.r and self.g == other.g and self.b == other.b and self.a == other.a

    def __add__(self, other):
        return color(self.r + other.r, self.g + other.g, self.b + other.b, self.a + other.a)

    def __sub__(self, other):
        return color(self.r - other.r, self.g - other.g, self.b - other.b, self.a - other.a)

    def __mul__(self, other):
        if isinstance(other, color):
            return color(self.r * other.r, self.g * other.g, self.b * other.b, self.a * other.a)
        else:
            return color(self.r * other, self.g * other, self.b * other, self.a * other)

    def __rmul__(self, other):
        return self * other

    @classproperty
    def black(self):
        return color(0,0,0,1)

    @classproperty
    def white(self):
        return color(1,1,1,1)

    @classproperty
    def red(self):
        return color(1,0,0,1)

    @classproperty
    def green(self):
        return color(0,1,0,1)

    @classproperty
    def blue(self):
        return color(0,0,1,1)

    @classproperty
    def cyan(self):
        return color(0,1,1,1)

    @classproperty
    def magenta(self):
        return color(1,0,1,1)

    @classproperty
    def yellow(self):
        return color(1,1,0,1)

    @classproperty
    def transparent(self):
        return color(1,1,1,0)

    @classproperty
    def miku(self):
        return color(0.22, 0.77, 0.73, 1)

    def __str__(self):
        return 'color({:.2f},{:.2f},{:.2f},{:.2f})'.format(self.r, self.g, self.b, self.a)

class color_o(color):
    __slots__ = ('__observer_callback', '__r', '__g', '__b', '__a')
    def __init__(self, observer_callback, r = 1, g = 1, b = 1, a = 1):
        self.__r = r
        self.__g = g
        self.__b = b
        self.__a = a
        self.__observer_callback = observer_callback

    def set(self, r, g, b, a = 1):
        self.__r = r
        self.__g = g
        self.__b = b
        self.__a = a
        self.__observer_callback()
        return self

    @property
    def r(self):
        return self.__r

    @r.setter
    def r(self, r):
        self.__r = r
        self.__observer_callback()
    
    @property
    def g(self):
        return self.__g
    
    @g.setter
    def g(self, g):
        self.__g = g
        self.__observer_callback()
    
    @property
    def b(self):
        return self.__b
    
    @b.setter
    def b(self, b):
        self.__b = b
        self.__observer_callback()
    
    @property
    def a(self):
        return self.__a
    
    @a.setter
    def a(self, a):
        self.__a = a
        self.__observer_callback()
