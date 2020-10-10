import ctypes
from api.capi import c_lib
from api.vec3 import vec3
from api.mesh import mesh
from api.shape import shape
import collections

c_lib.navigation_build.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.navigation_nearest_point.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p)
c_lib.navigation_farthest_point.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p)
c_lib.navigation_path.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_void_p, ctypes.c_int)

max_path = 128
c_path_data = ctypes.c_float * (max_path * 3)
path_data = c_path_data()

tmp_x = ctypes.c_float(0)
tmp_xp = ctypes.byref(tmp_x)
tmp_y = ctypes.c_float(0)
tmp_yp = ctypes.byref(tmp_y)
tmp_z = ctypes.c_float(0)
tmp_zp = ctypes.byref(tmp_z)

class navigation:
    __slots__ = ('__id', '__debug')
    def __init__(self, resname = None):
        self.__id = c_lib.navigation_create()
        self.__debug = False
        if resname is not None:
            self.load(resname)

    def load(self, resname):
        return c_lib.navigation_load(self.__id, resname.encode())

    class params:
        __slots__ = ('cell_size', 'cell_height', 'agent_height', 'agent_radius', 'max_climb', 'max_slope', 'min_region_size', 'merged_region_size', 'max_edge_length', 'max_edge_error')
        def __init__(self):
            self.cell_size = 0.3
            self.cell_height = 0.2

            self.agent_height = 1.6
            self.agent_radius = 0.3
            self.max_climb = 0.1
            self.max_slope = 45

            self.min_region_size = 8
            self.merged_region_size = 20

            self.max_edge_length = 12
            self.max_edge_error = 1.3

    def create(self, src, params = None):
        if params is None:
            params = navigation.params()
        def add(id, s):
            if isinstance(s, mesh):
                c_lib.navigation_add_mesh(id, s._mesh__id)
            elif isinstance(s, shape):
                c_lib.navigation_add_shape(id, s._shape__id)
            elif type(s) is not str and isinstance(s, collections.Sequence):
                for ss in src:
                    add(id, ss)
            else:
                c_lib.navigation_clear(id)
                raise ValueError("Unable to build navigation from " + type(s))
        c_lib.navigation_clear(self.__id)
        add(self.__id, src)
        return c_lib.navigation_build(self.__id, params.cell_size, params.cell_height, params.agent_height, params.agent_radius, params.max_climb, params.max_slope, params.min_region_size, params.merged_region_size, params.max_edge_length, params.max_edge_error)

    def nearest(self, pos, radius = 1):
        if c_lib.navigation_nearest_point(self.__id, pos.x, pos.y, pos.z, radius, tmp_xp, tmp_yp, tmp_zp):
            return vec3(tmp_x.value, tmp_y.value, tmp_z.value)
        return None

    def farthest(self, pos, dir, radius):
        if c_lib.navigation_farthest_point(self.__id, pos.x, pos.y, pos.z, dir.x, dir.z, radius, tmp_xp, tmp_yp, tmp_zp):
            return vec3(tmp_x.value, tmp_y.value, tmp_z.value)
        return None

    def path(self, pos_from, pos_to):
        count = c_lib.navigation_path(self.__id, pos_from.x, pos_from.y, pos_from.z, pos_to.x, pos_to.y, pos_to.z, path_data, max_path)
        if count <= 0:
            return None
        result = []
        for i in range(0, count * 3, 3):
            result.append(vec3(path_data[i], path_data[i + 1], path_data[i + 2]))
        return result

    @property
    def debug(self):
        return self.__debug

    @debug.setter
    def debug(self, enable):
        if self.__debug != enable:
            self.__debug = enable
            c_lib.navigation_set_debug(self.__id, enable)

    def __del__(self):
        c_lib.navigation_remove(self.__id)
