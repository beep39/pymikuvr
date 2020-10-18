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
from api.sys import sys
from api.texture import texture
from api.transform import transform
from api.ui import ui
from api.vec2 import vec2
from api.vec3 import vec3
from api.video import video

import gc
import random
import math
import traceback
import os.path

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

class script:
    __slots__ = ('autoreload', '__local_vars', '__global_vars',
                 '__script_path', '__script_path_is_local', '__script_modified_time',)
    def __init__(self):
        self.__local_vars = None
        self.__global_vars = None
        self.__script_path = None
        self.__script_path_is_local = True
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

        self.__local_vars = {}
        self.__global_vars = None
        updater.reset()
        render.reset()
        player.reset()

        if cache is None:
            gc.collect()

        text = None

        if local_fs:
            text = sys.load_text(name)
        else:
            try:
                with open(name, encoding='utf-8') as file:
                    text = file.read()
            except Exception as e:
                print("Script load error:", e)

        if text is None:
            print("Script not found:", name)
            return

        sys.reset_resources()

        self.__script_path = name
        self.__script_modified_time = self.get_modified_time()

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
            "sys": sys,
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
            sys.error(str(e) + "\n" + traceback.format_exc())
            return

        try:
            exec(code, self.__global_vars, self.__local_vars)
        except Exception as e:
            sys.error(str(e) + "\n" + traceback.format_exc())

        cache = None
        gc.collect()

    def reload(self):
        self.load(self.__script_path, self.__script_path_is_local)

    def check_for_changes(self):
        if not self.autoreload or self.__script_path is None:
            return

        mtime = self.get_modified_time()
        if mtime != self.__script_modified_time:
            self.__script_modified_time = mtime
            self.reload()

    def get_modified_time(self):
        #ToDo
        if self.__script_path_is_local:
            return

        try:
            return os.path.getmtime(self.__script_path)
        except Exception as e:
            print(e)
        return None

scene_script = script()
