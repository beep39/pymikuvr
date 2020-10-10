import ctypes
from api.capi import c_lib
from api.vec3 import vec3_o
from api.quat import quat_o

c_lib.transform_set_pos.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.transform_set_rot.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.transform_set_local_pos.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.transform_set_local_rot.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)

tmp_x = ctypes.c_float(0)
tmp_xp = ctypes.byref(tmp_x)
tmp_y = ctypes.c_float(0)
tmp_yp = ctypes.byref(tmp_y)
tmp_z = ctypes.c_float(0)
tmp_zp = ctypes.byref(tmp_z)
tmp_w = ctypes.c_float(0)
tmp_wp = ctypes.byref(tmp_w)

class transform:
    __slots__ = ('__id', '__parent', '__children', '__pos', '__rot', '__local_pos', '__local_rot')
    def __init__(self, id = -1):
        if id < 0:
            self.__id = c_lib.transform_create()
        else:
            self.__id = id

        self.__parent = None
        self.__children = []

        def update_pos():
            pos = self.__pos
            c_lib.transform_set_pos(self.__id, pos.x, pos.y, pos.z)
        self.__pos = vec3_o(update_pos)

        def update_rot():
            rot = self.__rot
            c_lib.transform_set_rot(self.__id, rot.x, rot.y, rot.z, rot.w)
        self.__rot = quat_o(update_rot)

        def update_local_pos():
            pos = self.__local_pos
            c_lib.transform_set_local_pos(self.__id, pos.x, pos.y, pos.z)
        self.__local_pos = vec3_o(update_local_pos)

        def update_local_rot():
            rot = self.__local_rot
            c_lib.transform_set_local_rot(self.__id, rot.x, rot.y, rot.z, rot.w)
        self.__local_rot = quat_o(update_local_rot)

    def __del__(self):
        c_lib.transform_remove(self.__id)

    @property
    def parent(self):
        return self.__parent

    @parent.setter
    def parent(self, parent):
        if self is parent:
            raise ValueError('unable to set parent to itself')

        if parent is self.__parent:
            return

        if parent is None:
            self.__parent.__children.remove(self)
            self.__parent = None
            c_lib.transform_set_parent(self.__id, -1)
        else:
            if not isinstance(parent, transform):
                raise ValueError('parent is not a transform')

            if c_lib.transform_set_parent(self.__id, parent.__id):
                self.__parent = parent
                parent.__children.append(self)
            else:
                raise ValueError('unable to set parent')

    def set_parent(self, parent, reset_local_transform = True):
        if parent is not None:
            parent.add_child(self, reset_local_transform)
        else:
            self.parent = None

    def add_child(self, child, reset_local_transform = True):
        child.parent = self
        if reset_local_transform:
            child.local_pos.set(0.0,0.0,0.0)
            child.local_rot.set(0.0,0.0,0.0,1.0)

    @property
    def children(self):
        return tuple(self.__children)

    # global transform

    @property
    def pos(self):
        c_lib.transform_get_pos(self.__id, tmp_xp, tmp_yp, tmp_zp)
        self.__pos._vec3_o__x = tmp_x.value
        self.__pos._vec3_o__y = tmp_y.value
        self.__pos._vec3_o__z = tmp_z.value
        return self.__pos

    @pos.setter
    def pos(self, pos):
        self.__pos.set(pos.x, pos.y, pos.z)

    @property
    def rot(self):
        c_lib.transform_get_rot(self.__id, tmp_xp, tmp_yp, tmp_zp, tmp_wp)
        self.__rot._quat_o__x = tmp_x.value
        self.__rot._quat_o__y = tmp_y.value
        self.__rot._quat_o__z = tmp_z.value
        self.__rot._quat_o__w = tmp_w.value
        return self.__rot

    @rot.setter
    def rot(self, rot):
        self.__rot.set(rot.x, rot.y, rot.z, rot.w)

    # local transform

    @property
    def local_pos(self):
        c_lib.transform_get_local_pos(self.__id, tmp_xp, tmp_yp, tmp_zp)
        self.__local_pos._vec3_o__x = tmp_x.value
        self.__local_pos._vec3_o__y = tmp_y.value
        self.__local_pos._vec3_o__z = tmp_z.value
        return self.__local_pos

    @local_pos.setter
    def local_pos(self, pos):
        self.__local_pos.set(pos.x, pos.y, pos.z)

    @property
    def local_rot(self):
        c_lib.transform_get_local_rot(self.__id, tmp_xp, tmp_yp, tmp_zp, tmp_wp)
        self.__local_rot._quat_o__x = tmp_x.value
        self.__local_rot._quat_o__y = tmp_y.value
        self.__local_rot._quat_o__z = tmp_z.value
        self.__local_rot._quat_o__w = tmp_w.value
        return self.__local_rot

    @local_rot.setter
    def local_rot(self, rot):
        self.__local_rot.set(rot.x, rot.y, rot.z, rot.w)
