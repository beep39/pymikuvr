import ctypes
from api.capi import c_lib
from api.base import base
from api.texture import texture

c_lib.camera_set_fov.argtypes = (ctypes.c_int, ctypes.c_float)

class camera(base):
    __slots__ = ('__id', '__texture', '__fps', '__fov')
    def __init__(self, w, h):
        self.__id = c_lib.camera_create()
        self.__texture = texture()
        self.__texture.create(w, h)
        self.__fps = None
        self.__fov = 60.0
        c_lib.camera_set_texture(self.__id, self.__texture._texture__id)
        def enable_callback(enable):
            c_lib.camera_set_enabled(self.__id, enable)
        super().__init__(c_lib.camera_get_origin(self.__id), enable_callback)

    @property
    def texture(self):
        return self.__texture

    @texture.setter
    def texture(self, texture):
        self.__texture = texture
        c_lib.camera_set_texture(self.__id, self.__texture._texture__id)

    @property
    def fov(self):
        return self.__fov

    @fov.setter
    def fov(self, v):
        self.__fov = v
        if v is None:
            v = -1
        c_lib.camera_set_fov(self.__id, v)

    @property
    def fps(self):
        return self.__fps

    @fps.setter
    def fps(self, v):
        self.__fps = v
        if v is None:
            v = -1
        c_lib.camera_set_fps(self.__id, v)

    def render_to(self, target):
        c_lib.camera_render_to(self.__id, target._texture__id)

    def __del__(self):
        c_lib.camera_remove(self.__id)
        super().__del__()
