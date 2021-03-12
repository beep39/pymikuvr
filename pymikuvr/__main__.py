from time import sleep, time
from sys import argv, exit, path
import os, gc

from api.render import render
from api.player import player
from api.script import scene_script
from api.system import system, system_internal
from api.base import updater
from api.ui import ui

if len(argv) < 2:
    exit("please specify game script to load, e.g.: python3 pymikuvr test.py")

system.argv = argv[2:]
system.default_res_folder = os.path.dirname(os.path.abspath(__file__)) + "/../resources/"
system.reset_resources()

title = "pymikuvr"

path = os.getcwd()
if system_internal.start_vr():
    print("starting in vr")
else:
    system_internal.start_window(640, 480, title)
    print("starting windowed")
os.chdir(path)

additional_glyphs = "伊弥皐睦叢磯朧曙漣霞嵐藤萩曾隈那珂阿矧笠智耶筑鹿榛洲捉瑞鶴葛鳳鷹龍驤翔"
ui.set_font("/font/SawarabiGothic-Regular.ttf", 32, 0.5, additional_glyphs)

scene_script.load(argv[1], False)

scripts_update = updater.update
player_update = player._update
native_update = system_internal.get_update_func()
frames = 0
start_time = time()
prev_time = start_time
while native_update():
    t = time()
    system.dt = t - prev_time
    system.time += system.dt
    prev_time = t
    player_update()
    scripts_update()

    frames += 1
    if (t - start_time) > 1 :
        scene_script.check_for_changes()
        render.fps = round(frames / (t - start_time))
        #print("FPS: ", render.fps)
        frames = 0
        start_time = t

print("exiting...")
gc.collect()
system_internal.exit()
