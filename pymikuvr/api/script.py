from api.animation import animation
from api.base import updater
from api.camera import camera
from api.color import color
from api.material import material
from api.mesh import mesh
from api.navigation import navigation
from api.particles import particles
from api.phys import phys
from api.player import player
from api.quat import quat
from api.render import render
from api.shape import shape
from api.sound import sound
from api.system import system
from api.texture import texture
from api.transform import transform
from api.ui import ui
from api.vec2 import vec2
from api.vec3 import vec3
from api.video import video

import gc
import random
import math
import imp
import traceback
import os.path
from sys import meta_path
from sys import modules

def tend(value, target, speedup, speeddn = None):
    if isinstance(target, vec3):
        return vec3.tend(value, target, speedup)
    if isinstance(target, quat):
        return quat.tend(value, target, speedup)
    diff = target - value
    if diff > speedup:
        return value + speedup;
    if speeddn is None:
        speeddn = speedup
    if -diff > speeddn:
        return value - speeddn;
    return target

def clamp(value, f, to):
    return max(f, min(value, to))

def load_text(name, local_fs):
    text = None
    if local_fs:
        text = system.load_text(name)
    else:
        try:
            with open(name, encoding='utf-8') as file:
                text = file.read()
        except Exception as e:
            print("Script load error:", e)
    return text

class importer_class(object):
    def __init__(self, local_fs, path, global_vars, local_vars):
        self.local_fs = local_fs
        self.path = path
        self.global_vars = global_vars
        self.local_vars = local_vars
        self.modules = []

    def full_path(self, name):
        return os.path.join(self.path, name.replace('.','/') + '.py')

    def full_folder_path(self, name):
        return os.path.join(self.path, name.replace('.','/'), '__init__.py')

    def find_module(self, name, path):
        if self.local_fs:
            raise NotImplementedError
        elif os.path.exists(self.full_path(name)) or os.path.exists(self.full_folder_path(name)):
            return self
        return None

    def load_module(self, name):
        if os.path.exists(self.full_folder_path(name)):
            new_module = imp.new_module(name)
            new_module.__path__ = name
            modules[name] = new_module
            self.modules.append(name);
            return new_module

        text = load_text(self.full_path(name), self.local_fs)
        if text is None:
            raise ImportError(name)

        code = None
        try:
            code = compile(text, name, 'exec')
        except Exception as e:
            system.error(str(e) + "\n" + traceback.format_exc())

        if code is None:
            raise ImportError(name)

        new_module = imp.new_module(name)

        try:
            exec(code, self.global_vars, new_module.__dict__)
        except Exception as e:
            system.error(str(e) + " in module " + name + "\n" + traceback.format_exc())
            new_module = None

        if new_module is None:
            raise ImportError(name)

        modules[name] = new_module
        self.modules.append(name);
        return new_module

    def cleanup(self):
        for m in self.modules:
            modules.pop(m)

class script:
    __slots__ = ('autoreload', '__local_vars', '__global_vars',
                 '__script_path', '__script_path_is_local', '__watch_paths',)
    def __init__(self):
        self.__local_vars = None
        self.__global_vars = None
        self.__script_path = None
        self.__script_path_is_local = True
        self.__watch_paths = []
        self.autoreload = True

    def load(self, name, local_fs = True):
        cache = None
        if name == self.__script_path and local_fs == self.__script_path_is_local:
            cache = (self.__local_vars, self.__global_vars)
            print("reloading script", name)
        else:
            print("loading script", name)

        self.__script_path = None
        self.__script_path_is_local = local_fs
        self.__watch_paths = []

        self.__local_vars = {}
        self.__global_vars = None
        updater.reset()
        render.reset()
        player.reset()

        if cache is None:
            gc.collect()

        text = load_text(name, local_fs)
        if text is None:
            print("Script not found:", name)
            return

        system.reset_resources()
        if not local_fs:
            system.add_resources_folder(os.path.dirname(name))

        self.__script_path = name

        self.__global_vars = {
            "animation": animation,
            "camera": camera,
            "color": color,
            "material": material,
            "mesh": mesh,
            "navigation": navigation,
            "particles": particles,
            "phys": phys,
            "player": player,
            "quat": quat,
            "render": render,
            "script": script,
            "shape": shape,
            "sound": sound,
            "system": system,
            "texture": texture,
            "transform": transform,
            "ui": ui,
            "vec2": vec2,
            "vec3": vec3,
            "video": video,
    
            "random": random,
            "math": math,

            "clamp": clamp,
            "tend": tend,
        }

        try:
            code = compile(text, name, 'exec')
        except Exception as e:
            system.error(str(e) + "\n" + traceback.format_exc())
            return

        importer = importer_class(local_fs, os.path.dirname(name), self.__global_vars, self.__local_vars)
        meta_path.append(importer)

        try:
            exec(code, self.__global_vars, self.__local_vars)
        except Exception as e:
            system.error(str(e) + "\n" + traceback.format_exc())

        if not self.__script_path_is_local:
            self.__watch_paths.append([self.__script_path, None]);
            for m in importer.modules:
                f = importer.full_folder_path(m)
                if os.path.exists(f):
                    self.__watch_paths.append([f, None])
                else:
                    self.__watch_paths.append([importer.full_path(m), None]);
            for p in self.__watch_paths:
                p[1] = self.get_modified_time(p[0])

        meta_path.remove(importer)
        importer.cleanup()

        cache = None
        gc.collect()

    def reload(self):
        self.load(self.__script_path, self.__script_path_is_local)

    def check_for_changes(self):
        if not self.autoreload:
            return

        reload = False
        for p in self.__watch_paths:
            mtime = self.get_modified_time(p[0])
            if mtime != p[1]:
                p[1] = mtime
                reload = True
        if reload:
            self.reload()

    def get_modified_time(self, path):
        if self.__script_path_is_local:
            raise NotImplementedError

        try:
            return os.path.getmtime(path)
        except Exception as e:
            print(e)
        return None

scene_script = script()
