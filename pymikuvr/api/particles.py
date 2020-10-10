from api.capi import c_lib
from api.base import base

class particles(base):
    __slots__ = ('__id')
    def __init__(self):
        raise NotImplementedError

        self.__id = c_lib.particles_create()
        def enable_callback(enable):
            c_lib.particles_set_enable(self.__id, enable)
        super().__init__(c_lib.particles_get_origin(self.__id), enable_callback)

    def __del__(self):
        raise NotImplementedError
        c_lib.particles_remove(self.__id)
