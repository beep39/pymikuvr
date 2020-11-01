from api.sys import sys
from api.transform import transform
import traceback

class updater_class:
    __slots__ = ('list', 'remove_list')
    def __init__(self):
        self.list = []
        self.remove_list = []

    def register(self, o):
        self.list.append(o)
        return

    def unregister(self, o):
        self.remove_list.append(o)
        return

    def update(self):
        if self.remove_list:
            for r in self.remove_list:
                try:
                    self.list.remove(r)
                except:
                    pass
            self.remove_list.clear()

        for o in self.list:
            if o.enabled and o.update is not None:
                try:
                    o.update(o)
                except Exception as e:
                    o.update = None
                    sys.error(str(e) + "\n" + traceback.format_exc())

    def reset(self):
        self.remove_list.clear()
        self.list.clear()

updater = updater_class()

class base(transform):
    __slots__ = ('name','__update', '__enabled', '__enable_callback')
    def __init__(self, origin_id, enable_callback = None):
        super().__init__(origin_id)
        self.name = None
        self.__update = None
        self.__enabled = True
        self.__enable_callback = enable_callback

    @property
    def enabled(self):
        return self.__enabled

    @enabled.setter
    def enabled(self, enable):
        if self.__enabled == enable:
            return
        self.__enabled = enable
        if self.__enable_callback is not None:
            self.__enable_callback(enable)

    @property
    def update(self):
        return self.__update

    @update.setter
    def update(self, u):
        if self.__update is not None:
            updater.unregister(self)
        self.__update = u
        if u is not None:
            updater.register(self)

    def set_update(self, function, interval = None, count = None):
        if interval is None and count is None:
            self.update = function
            return

        if count is None:
            time = 0
            def update_interval(obj):
                nonlocal time
                nonlocal function
                time += sys.dt
                while time > interval:
                    time -= interval
                    function(obj)
            self.update = update_interval
            return

        if interval is None:
            def update_count(obj):
                nonlocal count
                nonlocal function
                count -= 1
                if count == 0:
                    obj.update = None
                function(obj)
            self.update = update_count
            return

        time = 0
        def update_count(obj):
            nonlocal count
            nonlocal time
            nonlocal function
            time += sys.dt
            while time > interval:
                time -= interval
                count -= 1
                if count == 0:
                    obj.update = None
                function(obj)
        self.update = update_count

    def __del__(self):
        if self.__update is not None:
            updater.unregister(self)
        super().__del__()
