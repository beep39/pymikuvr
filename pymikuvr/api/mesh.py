import ctypes
from api.capi import c_lib
from api.base import base
from api.transform import transform
from api.animation import animation
from collections.abc import Mapping
from collections.abc import MutableSequence

c_lib.mesh_get_morph.restype = ctypes.c_float
c_lib.mesh_set_morph.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_float, ctypes.c_bool)
c_lib.mesh_get_morph_name.restype = ctypes.c_char_p
c_lib.mesh_get_bone_name.restype = ctypes.c_char_p
c_lib.mesh_get_material_name.restype = ctypes.c_char_p
c_lib.mesh_set_bone_pos.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)
c_lib.mesh_set_bone_rot.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool)

class bone_transform(transform):
    __slots__ = ('__release_id')
    def __init__(self, id, parent):
        super().__init__(id)
        self._transform__parent = parent
        parent._transform__children.append(self)

    @property
    def parent(self):
        return super().parent

    @parent.setter
    def parent(self, parent):
        raise ValueError('unable to set parent for bone')

    def __del__(self):
        super().__del__()

class mesh_bones(Mapping):
    def __init__(self, id, data, parent):
        self.__id = id
        self.__parent = parent
        self.__data = data

    def __getitem__(self, key):
        item = self.__data[key]
        if item is not None:
            return item

        item = bone_transform(c_lib.mesh_get_bone(self.__id, key.encode()), self.__parent)
        self.__data[key] = item
        return item

    def __len__(self):
        return len(self.__data)

    def __iter__(self):
        return iter(self.__data)

    def __str__(self):
        return str(list(self.keys()))

class mesh_morphs(Mapping):
    def __init__(self, id, data):
        self.__id = id
        self.__data = data

    def __getitem__(self, key):
        if not key in self.__data:
            raise KeyError(key)
        return c_lib.mesh_get_morph(self.__id, key.encode())

    def __len__(self):
        return len(self.__data)

    def __iter__(self):
        return iter(self.__data)

    def __str__(self):
        return str(list(self.keys()))

class mesh_animations(MutableSequence):
    __slots__ = ('__id', '__list')
    def __init__(self, id):
        self.__id = id
        self.__list = list()

    def __len__(self):
        return len(self.__list)

    def __getitem__(self, ii):
        return self.__list[ii]

    def __setitem__(self, ii, anim):
        anim = animation(anim)
        self.__list[ii] = anim
        c_lib.mesh_set_animation(self.__id, anim._animation__id, ii)

    def __delitem__(self, ii):
        del self.__list[ii]
        for i in range(ii, len(self.__list)):
            c_lib.mesh_set_animation(self.__id, self.__list[i]._animation__id, i)
        c_lib.mesh_set_animation(self.__id, -1, len(self.__list))

    def insert(self, ii, anim):
        anim = animation(anim)
        self.__list.insert(ii, anim)
        for i in range(ii, len(self.__list)):
            c_lib.mesh_set_animation(self.__id, self.__list[i]._animation__id, i)

class mesh_material:
    __slots__ = ('__id', '__idx', '__visible', '__texture', '__name')
    def __init__(self, id, idx):
        self.__id = id
        self.__idx = idx
        self.__visible = True
        self.__texture = None
        self.__name = None

    @property
    def name(self):
        if self.__name is not None:
            return self.__name
        self.__name = c_lib.mesh_get_material_name(self.__id, self.__idx).decode()
        return self.__name

    @property
    def enabled(self):
        return self.__visible

    @enabled.setter
    def enabled(self, v):
        self.__visible = v
        c_lib.mesh_material_visible(self.__id, self.__idx, v)

    @property
    def texture(self):
        if self.__texture is not None:
            return self.__texture
        self.__texture = texture()
        c_lib.mesh_init_texture(self.__id, self.__idx, self.__texture._texture__id)

    @texture.setter
    def texture(self, t):
        self.__texture = t
        c_lib.mesh_set_texture(self.__id, self.__idx, t._texture__id)

class mesh(base):
    __slots__ = ('__id', '__animations', '__morphs', '__materials', '__bones')
    def __init__(self, resource = None):
        self.__id = c_lib.mesh_create()
        self.__animations = mesh_animations(self.__id)
        self.__morphs = None
        self.__materials = None
        self.__bones = None
        def enable_callback(enable):
            c_lib.mesh_set_enabled(self.__id, enable)
        super().__init__(c_lib.mesh_get_origin(self.__id), enable_callback)
        if resource:
            self.load(resource)

    def load(self, resource):
        self.__animations = mesh_animations(self.__id)
        self.__morphs = None
        self.__materials = None
        self.__bones = None
        return c_lib.mesh_load(self.__id, resource.encode())

    @property
    def animation(self):
        if len(self.__animations) == 0:
            a = animation()
            self.__animations.append(a)
        return self.__animations[0]

    @animation.setter
    def animation(self, anim):
        if len(self.__animations) == 0:
            self.__animations.append(anim)
        else:
            self.__animations[0] = anim

    @property
    def animations(self):
        return self.__animations

    @property
    def bones(self):
        if self.__bones:
            return self.__bones

        bone_names = []
        count = c_lib.mesh_get_bones_count(self.__id)
        for i in range(count):
            bone_names.append(c_lib.mesh_get_bone_name(self.__id, i).decode())
        self.__bones = mesh_bones(self.__id, {item : None for item in bone_names}, self)
        return self.__bones

    def bone(self, name, pos = None, rot = None, additive = False):
        if pos is None and rot is None:
            return self.bones[name]
        if pos is not None:
            c_lib.mesh_set_bone_pos(self.__id, name.encode(), pos.x, pos.y, pos.z, additive)
        if rot is not None:
            c_lib.mesh_set_bone_rot(self.__id, name.encode(), rot.x, rot.y, rot.z, rot.w, additive)

    @property
    def morphs(self):
        if self.__morphs:
            return self.__morphs

        morph_names = []
        count = c_lib.mesh_get_morphs_count(self.__id)
        for i in range(count):
            morph_names.append(c_lib.mesh_get_morph_name(self.__id, i).decode())

        self.__morphs = mesh_morphs(self.__id, {item : None for item in morph_names})
        return self.__morphs

    def morph(self, name, value = None, override_animation = True):
        if value is None:
            m = c_lib.mesh_get_morph(self.__id, name.encode())
            if m < -1000:
                return None
            return m
        c_lib.mesh_set_morph(self.__id, name.encode(), value, override_animation)

    @property
    def materials(self):
        if self.__materials:
            return tuple(self.__materials)

        count = c_lib.mesh_get_materials_count(self.__id)
        self.__materials = []
        for i in range(count):
            self.__materials.append(mesh_material(self.__id, i))
        return self.__materials

    def __del__(self):
        c_lib.mesh_remove(self.__id)
        super().__del__()
