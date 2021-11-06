from api.base import updater
from api.vec2 import vec2
from api.vec3 import vec3
from api.system import system

import math

class locomotion:
    __slots__ = ('enabled', 'update', '_target', '_walk_update', '_turn_update')
    def __init__(self, target):
        self._target = target
        self.enabled = True
        def empty():
            return
        self._walk_update = empty
        self._turn_update = empty
        def update(o):
            o._walk_update()
            o._turn_update()
        self.update = update

    def walk(self, source, speed=3, area=None):
        if area is not None:
            raise NotImplementedError

        updater.register(self)

        pos = self._target.pos
        head = self._target.head
        def update():
            s = vec2(source)
            l = s.length()
            if l > 1:
                s /= l
            s.y = -s.y
            s = s.rotated(head.rot.yaw) * (system.dt * speed)
            pos.x += s.x
            pos.z += s.y
        self._walk_update = update

    def fly(self, source, speed=3, area=None):
        if area is not None:
            raise NotImplementedError

        updater.register(self)

        t = self._target
        head = self._target.head
        def update():
            s = vec3(source.x, 0, -source.y)
            l = s.length()
            if l > 1:
                s /= l
            t.pos += head.rot * s * (system.dt * speed)
        self._walk_update = update

    def turn(self, source, speed=1, fixed_angle=None):
        updater.register(self)

        speed *= 360
        t = self._target
        if fixed_angle is None:
            def update():
                t.rot.yaw -= source.x * system.dt * speed
            self._turn_update = update
            return

        rotating = False
        rotating_delta = 0
        def update():
            nonlocal rotating
            nonlocal rotating_delta
            lr = source.x
            if not rotating:
                if abs(lr) > 0.5:
                    rotating_delta = -math.copysign(fixed_angle, lr)
                    rotating = True
            elif abs(lr) < 0.2:
                rotating = False
            else:
                 rotating = True
            if rotating_delta != 0:
                drot = system.dt * speed
                if abs(rotating_delta) > drot:
                    drot = math.copysign(drot, rotating_delta)
                    t.rot.yaw += drot
                    rotating_delta -= drot
                else:
                    t.rot.yaw += rotating_delta
                    rotating_delta = 0
        self._turn_update = update
