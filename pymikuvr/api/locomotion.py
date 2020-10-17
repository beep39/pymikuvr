from api.base import updater
from api.transform import transform
from api.vec2 import vec2
from api.vec3 import vec3
from api.sys import sys

class locomotion:
    __slots__ = ('_target', 'enabled', 'update')
    def __init__(self, target):
        self._target = target
        self.enabled = True
        def empty(o):
            return
        self.update = empty
        updater.register(self)

    def walk(self, speed, source, area=None):
        if area is not None:
            raise NotImplementedError

        pos = self._target.pos
        head = self._target.head
        def walk_update(o):
            s = vec2(source)
            l = s.length()
            if l > 1:
                s /= l
            s.y = -s.y
            s = s.rotated(head.rot.yaw) * (sys.dt * speed)
            pos.x += s.x
            pos.z += s.y
        self.update = walk_update

    def fly(self, speed, source, area=None):
        if area is not None:
            raise NotImplementedError

        t = self._target
        head = self._target.head
        def fly_update(o):
            s = vec3(source.x, 0, -source.y)
            l = s.length()
            if l > 1:
                s /= l
            t.pos += head.rot * s * (sys.dt * speed)
        self.update = fly_update
