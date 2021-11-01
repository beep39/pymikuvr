import ctypes
import os.path
import platform

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
