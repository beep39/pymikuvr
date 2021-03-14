import ctypes
from api.capi import c_lib
from api.color import color_o
from api.vec3 import vec3_o

c_lib.render_light_ambient.argtypes = (ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.render_light_color.argtypes = (ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.render_light_dir.argtypes = (ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.render_shadows_cascades.argtypes = (ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)
c_lib.render_set_shadows_bias.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float)
c_lib.render_set_znearfar.argtypes = (ctypes.c_float, ctypes.c_float)

class directional_light:
    slots = ('__ambient', '__color', '__intensity', '__dir')
    def __init__(self):
        def update_ambient():
            c_lib.render_light_ambient(self.__ambient.r, self.__ambient.g, self.__ambient.b)
        self.__ambient = color_o(update_ambient, 0.5, 0.5, 0.5)
        update_ambient()
        def update_color():
            c_lib.render_light_color(self.__color.r * self.__intensity, self.__color.g * self.__intensity, self.__color.b * self.__intensity)
        self.__color = color_o(update_color)
        self.__intensity = 0.6
        update_color()
        def update_dir():
            c_lib.render_light_dir(self.__dir.x, self.__dir.y, self.__dir.z)
        self.__dir = vec3_o(update_dir, 0.4, -0.82, -0.4)
        update_dir()

    @property
    def ambient(self):
        return self.__ambient

    @ambient.setter
    def ambient(self, v):
        self.__ambient.set(v.r, v.g, v.b)

    @property
    def color(self):
        return self.__color

    @color.setter
    def color(self, v):
        self.__color.set(v.r, v.g, v.b)

    @property
    def intensity(self):
        return self.__intensity

    @intensity.setter
    def intensity(self, v):
        self.__intensity = v
        self.__color.set(self.__color.r, self.__color.g, self.__color.b)

    @property
    def dir(self):
        self.__dir.normalize()
        return self.__dir

    @dir.setter
    def dir(self, v):
        self.__dir.set(v.x, v.y, v.z)

class shadows:
    slots = ('__enabled', '__resolution', '__size')
    def __init__(self):
        self.__enabled = True
        self.__resolution = 0
        self._resolution = 4096
        self.__cascades = None
        self._cascades = (4, 20, 70, 300)
        self.__bias = None
        self._bias = ((1,7), (1,7), (1,7), (1,128))

    @property
    def enabled(self):
        return self.__enabled

    @enabled.setter
    def enabled(self, v):
        if self.__enabled == v:
            return
        self.__enabled = v
        c_lib.render_shadows_enabled(v)

    @property
    def _resolution(self):
        return self.__resolution

    @_resolution.setter
    def _resolution(self, v):
        v = int(v)
        if self.__resolution == v:
            return
        self.__resolution = v
        c_lib.render_shadows_resolution(v)

    @property
    def _cascades(self):
        return self.__size

    @_cascades.setter
    def _cascades(self, v):
        if len(v) > 4:
            raise ValueError('shadow cascades count is limited to 4')

        self.__cascades = tuple(v)

        c = [0, 0, 0, 0]
        for i in range(len(v)):
            c[i] = v[i]
        c_lib.render_shadows_cascades(c[0], c[1], c[2], c[3])

    @property
    def _bias(self):
        return self.__bias

    @_cascades.setter
    def _bias(self, bias):
        for i in range(len(bias)):
            c_lib.render_set_shadows_bias(i, bias[i][0], bias[i][1])
        self.__bias = bias

class pipeline:
    def __init__(self):
        self.load("pipeline.txt")

    def load(self, resname):
        c_lib.render_pipeline_load(resname.encode())
    def flag(self, name, v = None):
        if v is None:
            c_lib.render_pipeline_get_condition(name.encode())
        else:
            c_lib.render_pipeline_set_condition(name.encode(), v)

class render_class:
    __slots__ = ('fps', '__light', '__shadows', '__pipeline')
    def __init__(self):
        self.fps = 0
        self.reset()

    @property
    def light(self):
        return self.__light

    @property
    def shadows(self):
        return self.__shadows

    @property
    def pipeline(self):
        return self.__pipeline

    def reset(self):
        self.__light = directional_light()
        self.__shadows = shadows()
        self.__pipeline = pipeline()
        self._set_znearfar(0.05, 300)

    def _set_znearfar(self, znear, zfar):
        c_lib.render_set_znearfar(znear, zfar)

render = render_class()
