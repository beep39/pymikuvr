import ctypes
from api.capi import c_lib
from api.base import base
import math

c_lib.sound_play2d.argtypes = (ctypes.c_char_p, ctypes.c_float)
c_lib.sound_play3d.argtypes = (ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.sound_set_volume.argtypes = (ctypes.c_int, ctypes.c_float)
c_lib.sound_set_pitch.argtypes = (ctypes.c_int, ctypes.c_float)
c_lib.sound_set_radius.argtypes = (ctypes.c_int, ctypes.c_float)

class sound(base):
    __slots__ = ('__id', '__volume', '__pitch', '__radius', '__duration')
    def __init__(self, resource = None, loop = False):
        self.__id = c_lib.sound_create()
        self.__volume = 1
        self.__pitch = 1
        self.__radius = None
        self.__duration = 0
        def enable_callback(enable):
            c_lib.sound_set_enabled(self.__id, enable)
        super().__init__(c_lib.sound_get_origin(self.__id), enable_callback)
        if resource:
            self.play(resource, loop)

    @staticmethod
    def play2d(resource, volume = 1):
        if volume <= 0:
            return False
        return c_lib.sound_play2d(resource.encode(), volume)

    @staticmethod
    def play3d(resource, pos, volume = 1, radius = None, pitch = 1):
        if volume <= 0:
            return False
        if radius is None:
            radius = -1
        return c_lib.sound_play3d(resource.encode(), pos.x, pos.y, pos.z, volume, pitch, radius)

    @staticmethod
    def preload(resource):
        return c_lib.sound_preload(resource.encode())

    def play(self, resource, loop):
        self.__duration = c_lib.sound_play(self.__id, resource.encode(), loop)* 0.001

    def stop(self):
        self.__duration = 0
        c_lib.sound_stop(self.__id)

    @property
    def level(self):
        #return c_lib.sound_get_level(self.__id) * 0.001
        return (math.exp(c_lib.sound_get_level(self.__id) * 0.001)-1) / (math.e-1)

    @property
    def time(self):
        return c_lib.sound_get_time(self.__id) * 0.001

    @time.setter
    def time(self, time):
        c_lib.sound_set_time(self.__id, int(time * 1000))

    @property
    def volume(self):
        return self.__volume

    @volume.setter
    def volume(self, v):
        self.__volume = v
        c_lib.sound_set_volume(self.__id, v)

    @property
    def pitch(self):
        return self.__pitch

    @pitch.setter
    def pitch(self, v):
        self.__pitch = v
        c_lib.sound_set_pitch(self.__id, v)

    @property
    def radius(self):
        return self.__radius

    @radius.setter
    def radius(self, v):
        self.__radius = v
        if radius is None:
            radius = -1
        c_lib.sound_set_radius(self.__id, v)

    @property
    def duration(self):
        return self.__duration

    def __del__(self):
        c_lib.sound_remove(self.__id)
        super().__del__()
