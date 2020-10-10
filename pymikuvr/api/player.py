import ctypes
from api.capi import c_lib
from api.base import base
from api.transform import transform
from api.vec2 import vec2
from api.vec3 import vec3_c
from api.quat import quat_c

c_lib.sys_get_ctrl.restype = ctypes.c_ulonglong

tmp_jx = ctypes.c_float(0)
tmp_jy = ctypes.c_float(0)
tmp_tx = ctypes.c_float(0)
tmp_ty = ctypes.c_float(0)
tmp_trigger = ctypes.c_float(0)
tmp_pjx = ctypes.byref(tmp_jx)
tmp_pjy = ctypes.byref(tmp_jy)
tmp_ptx = ctypes.byref(tmp_tx)
tmp_pty = ctypes.byref(tmp_ty)
tmp_ptrigger = ctypes.byref(tmp_trigger)

class ctrl_knuckle:
    __slots__ = ('__right','__btn','stick','touch','trigger')
    def __init__(self, right):
        self.__right = right
        self.__btn = 0
        self.stick = vec2()
        self.touch = vec2()
        self.trigger = 0

    def _update(self):
        self.__btn = c_lib.sys_get_ctrl(self.__right, tmp_pjx, tmp_pjy, tmp_ptx, tmp_pty, tmp_ptrigger)
        self.stick.x = tmp_jx.value
        self.stick.y = tmp_jy.value
        self.touch.x = tmp_tx.value
        self.touch.y = tmp_ty.value
        self.trigger = tmp_trigger.value

    @property
    def a(self):
        return self.__btn & (1 << 2)

    @property
    def b(self):
        return self.__btn & (1 << 1)
 
    @property
    def touch_btn(self):
        return self.__btn & (1 << 32)
 
    @property
    def trigger_btn(self):
        return self.__btn & (1 << 33)
 
    @property
    def stick_btn(self):
        return self.__btn & (1 << 35)

class transform_ro(transform):
    __slots__ = ()
    def __init__(self, id, parent):
        super().__init__(id)
        self._transform__parent = parent
        parent._transform__children.append(self)

    @property
    def parent(self):
        return super().parent

    @parent.setter
    def parent(self, parent):
        raise ValueError('unable to set parent for this transform')

    @property
    def pos(self):
        return vec3_c(super().pos)

    @property
    def rot(self):
        return quat_c(super().rot)

    @property
    def local_pos(self):
        return vec3_c(super().local_pos)

    @property
    def local_rot(self):
        return quat_c(super().local_rot)

    def __del__(self):
        return

class player_class(base):
    __slots__ = ('head', 'left_hand', 'right_hand', 'left_ctrl', 'right_ctrl')
    def __init__(self):
        super().__init__(c_lib.player_get_transform(b"origin"))
        self.head = transform_ro(c_lib.player_get_transform(b"head"), self)
        self.left_hand = transform_ro(c_lib.player_get_transform(b"lhand"), self)
        self.right_hand = transform_ro(c_lib.player_get_transform(b"rhand"), self)
        self.left_ctrl = ctrl_knuckle(False)
        self.right_ctrl = ctrl_knuckle(True)

    def _update(self):
        self.left_ctrl._update()
        self.right_ctrl._update()

    def reset(self, reset_position = False):
        self.enabled = True
        self.update = None
        for c in self.head.children:
            c.parent = None
        for c in self.left_hand.children:
            c.parent = None
        for c in self.right_hand.children:
            c.parent = None
        if reset_position:
            self.pos = vec3()
            self.rot = quat()

    def __del__(self):
        return

player = player_class()
