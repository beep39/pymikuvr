import ctypes
from api.capi import c_lib
from api.color import color, color_o
from api.texture import texture
from api.vec3 import vec3, vec3_o

c_lib.material_set_param.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.material_set_opaque.argtypes = (ctypes.c_int, ctypes.c_float)

def unpack_material(init_f):
    def _wrapper(self, *args, **kws):
        if args and isinstance(args[0], material_base):
            return init_f(self, _copy_from=args[0])
        else:
            return init_f(self, *args, **kws)
    return _wrapper

class material_base:
    slots = ('__id', '__params', '__textures', '__color')
    @unpack_material
    def __init__(self, resname, tex = None, _copy_from = None):
        self.__id = c_lib.material_create()
        self.__params = {}
        self.__textures = {}
        if _copy_from is None:
            self.load(resname)
            self.color = color.white
            if tex is not None:
                if isinstance(tex, texture):
                    self.texture = tex
                else:
                    self.texture = texture(tex)
        else:
            c_lib.material_copy(_copy_from.__id, self.__id)
            for k,v in _copy_from.__params.items():
                self.set_param(k, v)
            for k,v in _copy_from.__textures.items():
                self.set_texture(k, v)

    def load(self, resname):
        self.__params = {}
        self.__textures = {}
        return c_lib.material_load(self.__id, resname.encode())

    @property
    def color(self):
        return self.__params["color"]

    @color.setter
    def color(self, c):
        self.set_param("color", c)

    @property
    def texture(self):
        try:
            return self._textures["diffuse"]
        except Exception:
            return None

    @texture.setter
    def texture(self, t):
        self.set_texture("diffuse", t)

    def set_param(self, name, value = None):
        return self._set_param(name, value, name == "color")

    def _set_param(self, name, value = None, affects_opaque = False):
        if isinstance(value, vec3):
            w = 0.0
            if name == "color":
                w = 1.0
                c_lib.material_set_opaque(self.__id, w)
            def update_param():
                v = self.__params[name]
                c_lib.material_set_param(self.__id, name.encode(), v.x, v.y, v.z, w)
            self.__params[name] = vec3_o(update_param)
            update_param()
        elif isinstance(value, color):
            def update_param():
                v = self.__params[name]
                c_lib.material_set_param(self.__id, name.encode(), v.r, v.g, v.b, v.a)
                if affects_opaque:
                    c_lib.material_set_opaque(self.__id, v.a)
            self.__params[name] = color_o(update_param, value.r, value.g, value.b, value.a)
            update_param()
        else:
            raise NotImplementedError

    def set_texture(self, name, value):
        if value is None:
            raise ValueError("texture value must not be null")
        self.__textures[name] = value
        c_lib.material_set_texture(self.__id, name.encode(), value._texture__id)

    def __del__(self):
        c_lib.material_remove(self.__id)

class material(material_base):
    @unpack_material
    def __init__(self, texture = None, _copy_from = None):
        def update_color():
            c = self._color
            self.set_param("amb k", c * 0.4)
            self.set_param("diff k", color(c.r * 0.6, c.g * 0.6, c.b * 0.6, c.a))
            self.set_param("color", c)
        self._color = color_o(update_color, 1, 1, 1, 1)
        super().__init__("materials/lambert.txt", texture, _copy_from)

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, c):
        self._color.set(c.r, c.g, c.b, c.a)

    def set_param(self, name, value = None):
        return self._set_param(name, value, name == "color" or name == "diff k")

    @staticmethod
    def additive(texture = None):
        return  material_base("materials/additive.txt", texture)

    @staticmethod
    def pbr():
        raise NotImplementedError

    @staticmethod
    def toon():
        raise NotImplementedError

    @staticmethod
    def unlit(texture = None):
        return material_base("materials/unlit.txt", texture)

    def __del__(self):
        super().__del__()
