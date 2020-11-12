import ctypes
import os.path
import platform

def load_lib():

    lib_path = os.path.normpath(os.path.dirname(os.path.abspath(__file__)) + "/../../lib")
    system = platform.system()
    if system == 'Windows':
        os.add_dll_directory(lib_path)
        return ctypes.CDLL('pymikuvr.dll')

    path = os.getcwd()
    os.chdir(lib_path)
    lib = ctypes.CDLL("libpymikuvr.dylib" if system == 'Darwin' else "libpymikuvr.so")
    os.chdir(path)
    return lib

c_lib = load_lib()
