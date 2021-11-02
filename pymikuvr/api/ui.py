import ctypes
from api.capi import c_lib, c_callback
from api.color import color
from api.base import base

c_lib.ui_set_font.argtypes = (ctypes.c_char_p, ctypes.c_int, ctypes.c_float, ctypes.c_char_p)
c_lib.ui_set_pivot.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float)
c_lib.ui_set_size.argtypes = (ctypes.c_int, ctypes.c_float, ctypes.c_float)
c_lib.ui_add_slider.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_float, ctypes.c_float)
c_lib.ui_set_slider.argtypes = (ctypes.c_int, ctypes.c_int, ctypes.c_float)
c_lib.ui_get_slider.restype = ctypes.c_float
c_lib.ui_get_list_value.restype = ctypes.c_char_p
c_lib.ui_set_color.argtypes = (ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float)

class ui_label:
    __slots__ = ('__id','__p','__text')
    def __init__(self, p, id, text):
        self.__text = text
        self.__p = p
        self.__id = id

    @property
    def text(self):
        return self.__text

    @text.setter
    def text(self, text):
        if text is None:
            text = ""
        if self.__text != text:
            self.__text = text
            c_lib.ui_set_text(self.__p._ui__id, self.__id, text.encode())

class ui_btn(ui_label):
    __slots__ = ()
    def __init__(self, p, id, text):
        super().__init__(p, id, text)

    @property
    def value(self):
        return c_lib.ui_get_bool(self._ui_label__p._ui__id, self._ui_label__id)

class ui_slider(ui_label):
    __slots__ = ()
    def __init__(self, p, id, text):
        super().__init__(p, id, text)

    @property
    def value(self):
        return c_lib.ui_get_slider(self._ui_label__p._ui__id, self._ui_label__id)

    @value.setter
    def value(self, v):
        c_lib.ui_set_slider(self._ui_label__p._ui__id, self._ui_label__id, v)

r = ctypes.c_float(0)
rp = ctypes.byref(r)
g = ctypes.c_float(0)
gp = ctypes.byref(g)
b = ctypes.c_float(0)
bp = ctypes.byref(b)
a = ctypes.c_float(0)
ap = ctypes.byref(a)

class ui_coloredit(ui_label):
    __slots__ = ()
    def __init__(self, p, id, text):
        super().__init__(p, id, text)

    @property
    def value(self):
        c_lib.ui_get_color(self._ui_label__p._ui__id, self._ui_label__id, rp, gp, bp, ap)
        return color(r.value, g.value, b.value, a.value)

    @value.setter
    def value(self, v):
        c_lib.ui_set_color(self._ui_label__p._ui__id, self._ui_label__id, v.r, v.g, v.b, v.a)

class ui_checkbox(ui_label):
    __slots__ = ()
    def __init__(self, p, id, text):
        super().__init__(p, id, text)

    @property
    def value(self):
        return c_lib.ui_get_bool(self._ui_label__p._ui__id, self._ui_label__id)

    @value.setter
    def value(self, v):
        c_lib.ui_set_bool(self._ui_label__p._ui__id, self._ui_label__id, v)

class ui_list:
    __slots__ = ('__items', '__p', '__id')
    def __init__(self, p, id, items):
        self.__p = p
        self.__id = id
        self.__items = items

    @property
    def value(self):
        return c_lib.ui_get_list_value(self.__p._ui__id, self.__id)

    @value.setter
    def value(self, v):
        if v is None:
            raise NotImplementedError
        c_lib.ui_set_list_value(self.__p._ui__id, self.__id, v.encode())

    @property
    def idx(self):
        return c_lib.ui_get_list_idx(self.__p._ui__id, self.__id)
    
    @idx.setter
    def idx(self, v):
        if v is None:
            raise NotImplementedError
        l = len(self.__items)
        if l == 0:
            return
        v = v % l
        if v < 0:
            v += l
        c_lib.ui_set_list_idx(self.__p._ui__id, self.__id, int(v))

    @property
    def items(self):
        return tuple(self.__items)

    @items.setter
    def items(self, new_items):
        items_encoded = [itm.encode() for itm in new_items]
        arr = (ctypes.c_char_p * len(items_encoded))()
        arr[:] = items_encoded
        c_lib.ui_set_list(self.__p._ui__id, self.__id, arr, len(items_encoded))
        self.__items = new_items

class ui(base):
    __slots__ = ('__id', '__w', '__h', '__px', '__py', '__callbacks')
    def __init__(self, caption = None, w = 0.35, h = 0.5, px = 0.5, py = 1, style = "light", ignore_input = False, no_background = False):
        if caption is None:
            caption = ""
        self.__id = c_lib.ui_create()
        self.__px = px
        self.__py = py
        self.__callbacks = []
        self.__w = w
        self.__h = h
        c_lib.ui_set_style(self.__id, style.encode())
        c_lib.ui_set_caption(self.__id, caption.encode())
        c_lib.ui_set_size(self.__id, self.__w, self.__h)
        c_lib.ui_set_pivot(self.__id, self.__px, self.__py)
        if ignore_input:
            c_lib.ui_set_ignore_input(self.__id, True)
        if no_background:
            c_lib.ui_set_background(self.__id, False)
        def enable_callback(enable):
            c_lib.ui_set_enabled(self.__id, enable)
        super().__init__(c_lib.ui_get_origin(self.__id), enable_callback)

    def __del__(self):
        c_lib.ui_remove(self.__id)
        super().__del__()

    @staticmethod
    def set_font(resname, size, scale, additional_gliphs = None):
        if additional_gliphs is None:
            additional_gliphs = ""
        return c_lib.ui_set_font(resname.encode(), size, scale, additional_gliphs.encode())

    @property
    def px(self):
        return self.__px

    @px.setter
    def px(self, v):
        self.__px = v
        c_lib.ui_set_pivot(self.__id, self.__px, self.__py)

    @property
    def py(self):
        return self.__py

    @py.setter
    def py(self, v):
        self.__py = v
        c_lib.ui_set_pivot(self.__id, self.__px, self.__py)

    @property
    def w(self):
        return self.__w

    @w.setter
    def w(self, v):
        self.__w = v
        c_lib.ui_set_size(self.__id, self.__w, self.__h)

    @property
    def h(self):
        return self.__h

    @h.setter
    def h(self, v):
        self.__h = v
        c_lib.ui_set_size(self.__id, self.__w, self.__h)

    def add_text(self, text, color = None):
        if color is not None:
            raise NotImplementedError
        id = c_lib.ui_add_text(self.__id, text.encode())
        return ui_label(self, id, text)

    def add_btn(self, text, callback = None):
        id = c_lib.ui_add_btn(self.__id, text.encode(), self._add_callback(callback))
        return ui_btn(self, id, text)

    def add_checkbox(self, text, callback = None, radio = False):
        id = c_lib.ui_add_checkbox(self.__id, text.encode(), self._add_callback(callback), radio)
        return ui_checkbox(self, id, text)

    def add_slider(self, text, callback = None, f = 0.0, t = 1.0):
        id = c_lib.ui_add_slider(self.__id, text.encode(), self._add_callback(callback), f, t)
        return ui_slider(self, id, text)

    def add_coloredit(self, text, callback = None, alpha = False):
        id = c_lib.ui_add_coloredit(self.__id, text.encode(), self._add_callback(callback), alpha)
        return ui_coloredit(self, id, text)

    def add_dropdown(self, items, callback = None):
        items_encoded = [itm.encode() for itm in items]
        arr = (ctypes.c_char_p * len(items_encoded))()
        arr[:] = items_encoded
        id = c_lib.ui_add_dropdown(self.__id, self._add_callback(callback), arr, len(items_encoded))
        return ui_list(self, id, items)

    def add_listbox(self, items, callback = None):
        items_encoded = [itm.encode() for itm in items]
        arr = (ctypes.c_char_p * len(items_encoded))()
        arr[:] = items_encoded
        id = c_lib.ui_add_listbox(self.__id, self._add_callback(callback), arr, len(items_encoded))
        return ui_list(self, id, items)

    def add_tab(self, text):
        c_lib.ui_add_tab(self.__id, text.encode())

    def add_spacing(self, separator = False):
        c_lib.ui_add_spacing(self.__id, separator)

    def add_hlayout(self, count):
        c_lib.ui_add_hlayout(self.__id, count)

    def add_scroll(self, count = -1):
        c_lib.ui_add_scroll(self.__id, count)

    def _add_callback(self, callback):
        if callback is None:
            return -1
        c = c_callback(callback)
        self.__callbacks.append(c)
        return c.id
