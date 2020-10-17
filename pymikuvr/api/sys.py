import ctypes
import platform
from api.capi import c_lib
from api.texture import texture

c_lib.sys_load_text.restype = ctypes.c_char_p

class sys_class:
    __slots__ = ('__screen_texture', 'default_res_folder', 'time', 'dt', 'argv', 'log', 'warnings', 'errors')
    def __init__(self):
        self.__screen_texture = None
        self.default_res_folder = None
        self.time = 0
        self.dt = 0
        self.argv = []
        self.log = []
        self.warnings = []
        self.errors = []

    @property
    def platform(self):
        return platform.system()

    @property
    def screen_texture(self):
        if self.__screen_texture is None:
            self.__screen_texture = texture()
            c_lib.sys_reg_desktop_texture(texture._texture__id)
        return self.__screen_texture

    def add_resources_folder(self, name):
        return c_lib.sys_add_resources_folder(name.encode())

    def reset_resources(self):
        c_lib.sys_reset_resources()
        if self.default_res_folder is not None:
            c_lib.sys_add_resources_folder(self.default_res_folder.encode())

    def load_text(self, name):
        ptr = c_lib.sys_load_text(name.encode())
        if ptr is None:
            return None
        text = ptr.decode()
        c_lib.sys_free_tmp()
        return text

    def verbose(self, *args):
        msg = ("{} " * (len(args)-1) + "{}").format(*args)
        self.log.append(msg)
        msg = msg.split("\n")
        msg = "\n| ".join(msg)
        print(msg)

    def warning(self, *args):
        msg = ("{} " * (len(args)-1) + "{}").format(*args)
        self.warnings.append(msg)
        self.verbose("Warning: " + msg)

    def error(self, *args):
        msg = ("{} " * (len(args)-1) + "{}").format(*args)
        self.errors.append(msg)
        self.verbose("Error: " + msg)

sys = sys_class()

class sys_internal_class:
    def start_vr(self):
        return c_lib.sys_start_vr()
    def start_window(self, width, height, title):
        return c_lib.sys_start_window(int(width), int(height), title.encode())
    def exit(self):
        c_lib.sys_exit()
    def get_update_func(self,):
        return c_lib.sys_update

sys_internal = sys_internal_class()
