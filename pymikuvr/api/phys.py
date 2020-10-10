import ctypes
from api.capi import c_lib
from api.base import base
from api.mesh import mesh
from api.shape import shape
import collections

c_lib.phys_set_box.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.phys_set_sphere.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float)
c_lib.phys_set_capsule.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.phys_set_cylinder.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.phys_set_cone.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.phys_build.argtypes = (ctypes.c_int, ctypes.c_float)
c_lib.phys_set_ground.argtypes = (ctypes.c_float, ctypes.c_bool)
c_lib.phys_trace_world.argtypes = (ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_float)
c_lib.phys_trace.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_void_p, ctypes.c_float)

tmp0 = ctypes.c_float(0)
tmp0p = ctypes.byref(tmp0)

class phys_obj(base):
    __slots__ = ('__id')
    def __init__(self):
        self.__id = c_lib.phys_create()
        def enable_callback(enable):
            c_lib.phys_set_enabled(self.__id, enable)
        super().__init__(c_lib.phys_get_origin(self.__id), enable_callback)

    def trace(self, pos, dir, max = 1000.0):
        if c_lib.phys_trace(self.__id, pos.x, pos.y, pos.z, dir.x, dir.y, dir.z, tmp0p, max):
            return tmp0.value
        return None

    def __del__(self):
        c_lib.phys_remove(self.__id)

class phys_class:
    __slots__ = ('__debug', '__ground')
    def __init__(self):
        self.__debug = False
        self.__ground = None

    def box(self, x, y, z, mass):
        if mass is None:
            mass = 0.0
        obj = phys_obj()
        c_lib.phys_set_box(obj._phys_obj__id, x, y, z, mass)
        return obj

    def sphere(self, r, mass):
        if mass is None:
            mass = 0.0
        obj = phys_obj()
        c_lib.phys_set_sphere(obj._phys_obj__id, r, mass)
        return obj

    def capsule(self, r, h, mass):
        if mass is None:
            mass = 0.0
        obj = phys_obj()
        c_lib.phys_set_capsule(obj._phys_obj__id, r, h, mass)
        return obj

    def cylinder(self, r, h, mass):
        if mass is None:
            mass = 0.0
        obj = phys_obj()
        c_lib.phys_set_cylinder(obj._phys_obj__id, r, h, mass)
        return obj

    def cone(self, r, h, mass):
        if mass is None:
            mass = 0.0
        obj = phys_obj()
        c_lib.phys_set_cone(obj._phys_obj__id, r, h, mass)
        return obj

    def create(self, src, mass):
        obj = phys_obj()
        if mass is None:
            mass = 0.0
        def add(id, s):
            if isinstance(s, mesh):
                if mass > 0:
                    c_lib.phys_clear(obj._phys_obj__id)
                    raise ValueError("triangle meshes with mass are not supported")
                c_lib.phys_add_mesh(id, s._mesh__id)
            elif isinstance(s, shape):
                c_lib.phys_add_shape(id, s._shape__id)
            elif type(s) is not str and isinstance(s, collections.Sequence):
                for ss in src:
                    add(id, ss)
            else:
                c_lib.phys_clear(obj._phys_obj__id)
                raise ValueError("Unable to create phys mesh from " + type(src))
        add(obj._phys_obj__id, src)
        if c_lib.phys_build(obj._phys_obj__id, mass):
            return obj
        return None

    @property
    def ground(self):
        return self.__ground

    @ground.setter
    def ground(self, y):
        if self.__ground == y:
            return
        self.__ground = y
        if y is None:
            c_lib.phys_set_ground(0, False)
        else:
            c_lib.phys_set_ground(y, True)

    def trace(self, pos, dir, max = 1000.0):
        if c_lib.phys_trace_world(pos.x, pos.y, pos.z, dir.x, dir.y, dir.z, tmp0p, 0, max):
            return tmp0.value
        return None

    @property
    def debug(self):
        return self.__debug

    @debug.setter
    def debug(self, enable):
        if self.__debug != enable:
            self.__debug = enable
            c_lib.phys_set_debug(enable)

phys = phys_class()
