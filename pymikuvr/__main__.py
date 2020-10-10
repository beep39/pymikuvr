from time import sleep, time
from sys import argv, exit, path
import os, gc

from api.render import render
from api.player import player
from api.script import scene_script
from api.sys import sys, sys_internal
from api.base import updater

if len(argv) < 2:
    exit("please specify game script to load, e.g.: python3 pymikuvr test.py")

sys.argv = argv[2:]
sys.default_res_folder = os.path.dirname(os.path.abspath(__file__)) + "/../resources/"
sys.reset_resources()

title = "pymikuvr"

path = os.getcwd()
if sys_internal.start_vr():
    print("starting in vr")
else:
    sys_internal.start_window(640, 480, title)
    print("starting windowed")
os.chdir(path)

scene_script.load(argv[1], False)

scripts_update = updater.update
player_update = player._update
native_update = sys_internal.get_update_func()
frames = 0
start_time = time()
prev_time = start_time
while native_update():
    t = time()
    sys.dt = t - prev_time
    sys.time += sys.dt
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
sys_internal.exit()
