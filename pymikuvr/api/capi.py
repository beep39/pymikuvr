import ctypes
import os.path
import platform
import json
import traceback

def load_lib():
    lib_path = os.path.normpath(os.path.dirname(os.path.abspath(__file__)) + "/../../lib")
    system = platform.system()
    if system == 'Windows':
        os.add_dll_directory(lib_path)
        return ctypes.CDLL('pymikuvr.dll')

    if system == 'Darwin':
        path = os.getcwd()
        os.chdir(lib_path)
        lib = ctypes.CDLL("libpymikuvr.dylib")
        os.chdir(path)
        return lib

    return ctypes.CDLL(os.path.join(lib_path, "libpymikuvr.so"))

c_lib = load_lib()

c_lib.sys_pop_callback.restype = ctypes.c_char_p

class callbacks_class:
    __slots__ = ('callbacks', 'free')
    def __init__(self):
        self.callbacks = []
        self.free = []

    def register(self, callback):
        if len(self.free) > 0:
            callback.id = self.free.pop()
        else:
            callback.id = len(self.callbacks)
            self.callbacks.append(callback)

    def unregister(self, callback):
        if self.callbacks[callback.id] is None:
            return

        self.callbacks[callback.id] = None
        self.free.append(callback.id)

    def update(self, on_error):
        event = c_lib.sys_pop_callback()
        while event is not None:
            args = json.loads(event)
            id = args.pop(0)
            if id < len(self.callbacks):
                cb = self.callbacks[id]
                if cb is not None and cb.call is not None:
                    try:
                        cb.call(*args)
                    except Exception as e:
                        on_error(str(e) + "\n" + traceback.format_exc())
            event = c_lib.sys_pop_callback()

callbacks = callbacks_class()

class c_callback:
    __slots__ = ('id', 'call')
    def __init__(self, callback = None):
        self.id = None
        self.call = callback
        callbacks.register(self)

    def __del__():
        callbacks.unregister(self)
