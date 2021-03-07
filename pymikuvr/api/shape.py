import ctypes
from api.capi import c_lib
from api.base import base
from api.color import color
from api.material import material
from api.texture import texture
from collections.abc import Sequence

c_lib.shape_set_sphere.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.shape_set_cylinder.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.shape_set_box.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.shape_set_plane.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.shape_set_heightmap.argtypes = (ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.shape_set_heightmap_tex.argtypes = (ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.shape_set_color.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.shape_trace.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_void_p)

tmp0 = ctypes.c_float(0)
tmp0p = ctypes.byref(tmp0)

class shape(base):
    __slots__ = ('__id', '__material')
    def __init__(self):
        self.__id = c_lib.shape_create()
        self.__material = material()
        c_lib.shape_set_material(self.__id, self.__material._material_base__id)
        def enable_callback(enable):
            c_lib.shape_set_enabled(self.__id, enable)
        super().__init__(c_lib.shape_get_origin(self.__id), enable_callback)

    def trace(self, pos, dir, max = None):
        if max is not None:
            raise NotImplementedError
        if c_lib.shape_trace(self.__id, pos.x, pos.y, pos.z, dir.x, dir.y, dir.z, tmp0p):
            return tmp0.value
        return None

    @staticmethod
    def sphere(r, s = 1, t = 1, normalize_tc = True):
        if normalize_tc != True:
            raise NotImplementedError
        sh = shape()
        c_lib.shape_set_sphere(sh.__id, r, s, t, normalize_tc)
        return sh

    @staticmethod
    def cylinder(r, h, s = 1, t = 1, normalize_tc = True):
        if s != 1 or t != 1 or normalize_tc != True:
            raise NotImplementedError
        sh = shape()
        c_lib.shape_set_cylinder(sh.__id, r, h, s, t, normalize_tc)
        return sh

    @staticmethod
    def box(x, y, z, s = 1, t = 1, normalize_tc = True):
        sh = shape()
        c_lib.shape_set_box(sh.__id, x, y, z, s, t, normalize_tc)
        return sh

    @staticmethod
    def plane(w, h, s = 1, t = 1, normalize_tc = True):
        sh = shape()
        c_lib.shape_set_plane(sh.__id, w, h, s, t, normalize_tc)
        return sh

    @staticmethod
    def heightmap(count_w, count_h, step, height, scale = 1, s = 1, t = 1, normalize_tc = True):
        if isinstance(height, Sequence):
            if isinstance(height[0], Sequence):
                if count_w is None:
                    count_w = len(height[0])
                if count_h is None:
                    count_h = len(height)
                if count_w > len(height[0]) or count_h > len(height):
                    raise ValueError("insufficent array size")
                return shape.heightmap(count_w, count_h, step, sum(height, []), scale, s, t , normalize_tc)
            else:
                l = len(height)
                if l < count_w * count_h:
                    raise ValueError("insufficent array size")
                sh = shape()
                arr = (ctypes.c_float * l)(*height)
                c_lib.shape_set_heightmap(sh.__id, count_w, count_h, step, arr, scale, s, t, normalize_tc)
                return sh
        elif callable(height):
            heights = []
            for i in range(count_h):
                for j in range(count_w):
                    heights.append(height(i, j))
            return shape.heightmap(count_w, count_h, step, heights, scale, s, t , normalize_tc)
        elif isinstance(height, texture):
            if height.w == 0:
                raise ValueError("invalid texture")
            if count_w is None:
                count_w = height.w
            if count_h is None:
                count_h = height.h
            print("texture!")
            sh = shape()
            c_lib.shape_set_heightmap_tex(sh.__id, count_w, count_h, step, height._texture__id, scale, s, t, normalize_tc)
            return sh
        else:
            raise ValueError("height must be 1d/2d array, texture, or a function(i,j)")

    @staticmethod
    def pyramide(x, h, z, s = 1, t = 1, normalize_tc = True):
        raise NotImplementedError
        sh = shape()
        c_lib.shape_set_pyramide(sh.__id, x, h, z, s, t, normalize_tc)
        return sh

    @staticmethod
    def capsule(r, h, s = 1, t = 1, normalize_tc = True):
        raise NotImplementedError
        sh = shape()
        c_lib.shape_set_capsule(sh.__id, r, h, s, t, normalize_tc)
        return sh

    @staticmethod
    def cone(r, h, s = 1, t = 1, normalize_tc = True):
        raise NotImplementedError
        sh = shape()
        c_lib.shape_set_cone(sh.__id, r, h, s, t, normalize_tc)
        return sh

    @staticmethod
    def torus(r, r2, s = 1, t = 1, normalize_tc = True):
        raise NotImplementedError
        sh = shape()
        c_lib.shape_set_torus(sh.__id, r, r2, s, t, normalize_tc)
        return sh

    def add(self, other, clear_original = True):
        if self is other:
            raise ValueError("unable to add shape to itself")
        c_lib.shape_add_shape(self.__id, other.__id)
        if clear_original:
            other.clear()

    @property
    def color(self):
        return self.__material.color

    @color.setter
    def color(self, c):
        self.__material.color = c

    @property
    def texture(self):
        return self.__material.texture

    @texture.setter
    def texture(self, t):
        self.__material.texture = t

    @property
    def material(self):
        return self.__material

    @material.setter
    def material(self, m):
        if m is not None:
            m = material(m)
            c_lib.shape_set_material(self.__id, m._material_base__id)
        else:
            c_lib.shape_set_material(self.__id, -1)
        self.__material = m

    def clear(self):
        c_lib.shape_remove(self.__id)
        self.__id = c_lib.shape_create()

    def __del__(self):
        c_lib.shape_remove(self.__id)
        super().__del__()
