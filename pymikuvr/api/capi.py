import ctypes
import os.path
import platform

def load_lib():
    lib_path = os.path.dirname(os.path.abspath(__file__)) + "/../../lib/"
    system = platform.system()
    if system == 'Windows':
        os.add_dll_directory(lib_path)
        return ctypes.CDLL('pymikuvr.dll')
    libname = "libpymikuvr.dylib" if system == 'Darwin' else "libpymikuvr.so"
    return ctypes.CDLL(lib_path + libname)

c_lib = load_lib()
