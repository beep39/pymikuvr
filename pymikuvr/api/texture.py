import ctypes
from api.capi import c_lib

c_lib.texture_build.argtypes = (ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)

class classproperty(property):
    def __get__(self, cls, owner):
        return classmethod(self.fget).__get__(None, owner)()

class texture:
    __slots__ = ('__id')
    def __init__(self, resource = None):
        self.__id = c_lib.texture_create()
        if resource:
            self.load(resource)

    def load(self, resource):
        if c_lib.texture_load(self.__id, resource.encode()):
            return True

        print("unable to load", resource)
        return False

    def create(self, w, h, color = None):
        if w <= 0:
            raise ValueError('Invalid width')
        if h <= 0:
            raise ValueError('Invalid height')
        if color is None:
            c_lib.texture_build(self.__id, int(w), int(h), 0.0, 0.0, 0.0, 1.0)
        else:
            c_lib.texture_build(self.__id, int(w), int(h), color.r, color.g, color.b, color.a)

    def save(self, resource):
        return c_lib.texture_save(self.__id, resource.encode())

    def copy(self):
        copy = texture()
        c_lib.texture_copy(self.__id, copy.__id)
        return copy

    @property
    def w(self):
        return c_lib.texture_get_width(self.__id)

    @property
    def h(self):
        return c_lib.texture_get_height(self.__id)

    def __eq__(self, other):
        return self.__id == other.__id

    def __del__(self):
        c_lib.texture_remove(self.__id)

    @classproperty
    def white(self):
        t = texture()
        c_lib.texture_white(t.__id)
        return t

    @classproperty
    def black(self):
        t = texture()
        c_lib.texture_black(t.__id)
        return t

    @classproperty
    def normal(self):
        t = texture()
        c_lib.texture_normal(t.__id)
        return t
