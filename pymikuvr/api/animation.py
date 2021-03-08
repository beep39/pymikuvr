import ctypes
from api.capi import c_lib
from api.sound import sound

c_lib.animation_set_speed.argtypes = (ctypes.c_int, ctypes.c_float)
c_lib.animation_set_weight.argtypes = (ctypes.c_int, ctypes.c_float)

def unpack_animation(init_f):
    def _wrapper(self, *args, **kws):
        if args and isinstance(args[0], animation):
            return init_f(self, _copy_from=args[0])
        else:
            return init_f(self, *args, **kws)
    return _wrapper

class animation:
    __slots__ = ('__id', '__loop', '__duration', '__speed', '__weight')
    @unpack_animation
    def __init__(self, resource = None, _copy_from = None):
        super().__init__()
        self.__id = c_lib.animation_create()
        if _copy_from is None:
            self.__loop = False
            self.__duration = 0
            self.__speed = 1.0
            self.__weight = 1.0
            if resource:
                self.load(resource)
        else:
            c_lib.animation_copy(_copy_from.__id, self.__id)
            self.__loop = _copy_from.__loop
            self.__duration = _copy_from.__duration
            self.__speed = _copy_from.__speed
            self.__weight = _copy_from.__weight

    def load(self, resource, loop = False):
        result = c_lib.animation_load(self.__id, resource.encode())
        if result < 0:
            self.__duration = 0
            return False
        else:
            self.__duration = result * 0.001
            self.loop = loop
            return True

    def set_range(self, range_from, range_to):
        self.__duration = c_lib.animation_set_range(self.__id, int(range_from * 1000), int(range_to * 1000)) * 0.001
        return self.__duration

    @property
    def loop(self):
        return self.__loop

    @loop.setter
    def loop(self, value):
        if self.__loop != value:
            self.__loop = value
            c_lib.animation_set_loop(self.__id, value)

    @property
    def time(self):
        return c_lib.animation_get_time(self.__id) * 0.001

    @time.setter
    def time(self, value):
        c_lib.animation_set_time(self.__id, int(value * 1000))

    @property
    def duration(self):
        return self.__duration

    @property
    def finished(self):
        return c_lib.animation_finished(self.__id)

    @property
    def speed(self):
        return self.__speed

    @speed.setter
    def speed(self, value):
        self.__speed = value
        c_lib.animation_set_speed(self.__id, value)

    @property
    def weight(self):
        return self.__weight

    @weight.setter
    def weight(self, value):
        self.__weight = value
        c_lib.animation_set_weight(self.__id, value)

    def mirror(self):
        time = self.time
        c_lib.animation_mirror(self.__id)
        c_lib.animation_set_weight(self.__id, self.__weight)
        c_lib.animation_set_speed(self.__id, self.__speed)
        c_lib.animation_set_loop(self.__id, self.__loop)
        self.time = time
        return self

    def __del__(self):
        c_lib.animation_remove(self.__id)
