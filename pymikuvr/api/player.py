import ctypes
from api.capi import c_lib
from api.base import base
from api.locomotion import locomotion
from api.transform import transform
from api.vec2 import vec2
from api.vec3 import vec3_c
from api.quat import quat_c

c_lib.sys_get_ctrl.restype = ctypes.c_uint
c_lib.sys_get_tracker.restype = ctypes.c_char_p

tmp_ax = ctypes.c_float(0)
tmp_ay = ctypes.c_float(0)
tmp_trigger = ctypes.c_float(0)
tmp_i = ctypes.c_int(0)
tmp_pax = ctypes.byref(tmp_ax)
tmp_pay = ctypes.byref(tmp_ay)
tmp_ptrigger = ctypes.byref(tmp_trigger)
tmp_pi = ctypes.byref(tmp_i)

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

class controller(transform_ro):
    __slots__ = ('__right','__btn','axis','trigger')
    def __init__(self, id, parent, right):
        super().__init__(id, parent)
        self.__right = right
        self.__btn = 0
        self.axis = vec2()
        self.trigger = 0

    def _update(self):
        self.__btn = c_lib.sys_get_ctrl(self.__right, tmp_pax, tmp_pay, tmp_ptrigger)
        self.axis.x = tmp_ax.value
        self.axis.y = tmp_ay.value
        self.trigger = tmp_trigger.value

    @property
    def hold(self):
        return self.__btn & (1 << 0) > 0

    @property
    def grip(self):
        return self.__btn & (1 << 1) > 0

    @property
    def menu(self):
        return self.__btn & (1 << 2) > 0

    @property
    def axis_btn(self):
        return self.__btn & (1 << 16) > 0

    @property
    def trigger_btn(self):
        return self.__btn & (1 << 17) > 0

class named_transform(transform_ro):
    __slots__ = ('__name')
    def __init__(self, name, id, parent):
        super().__init__(id, parent)
        self.__name = name

    @property
    def name(self):
        return self.__name

class player_class(base):
    __slots__ = ('head', 'left_hand', 'right_hand', 'locomotion', '__trackers')
    def __init__(self):
        super().__init__(c_lib.player_get_transform(b"origin"))
        self.head = transform_ro(c_lib.player_get_transform(b"head"), self)
        self.left_hand = controller(c_lib.player_get_transform(b"lhand"), self, False)
        self.right_hand = controller(c_lib.player_get_transform(b"rhand"), self, True)
        self.__trackers = []
        self.locomotion = locomotion(self)

    def _update(self):
        self.left_hand._update()
        self.right_hand._update()

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
        self.locomotion = locomotion(self)

    @property
    def trackers(self):
        count = c_lib.sys_get_trackers_count()
        off = len(self.__trackers)
        if off == count:
            return tuple(self.__trackers);
        for i in range(off, count):
            name = c_lib.sys_get_tracker(i, tmp_pi).decode()
            t = named_transform(name, tmp_i.value, self)
            self.__trackers.append(t)
        return tuple(self.__trackers);

    def __del__(self):
        return

player = player_class()
