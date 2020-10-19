import ctypes
from api.capi import c_lib
from api.texture import texture

ydl = None
try:
    import youtube_dl
    ydl = youtube_dl.YoutubeDL({'quiet': True})
except ImportError:
    pass

c_lib.video_set_volume.argtypes = (ctypes.c_int, ctypes.c_float)

class video:
    __slots__ = ('__id', '__texture', '__volume')
    def __init__(self, resname = None, loop = False):
        self.__id = c_lib.video_create()
        self.__texture = texture()
        self.__volume = 1.0
        c_lib.video_set_texture(self.__id, self.__texture._texture__id)
        if resname is not None:
            self.play(resname, loop)

    def play(self, resname, loop = False, external_audio = None):
        self.stop()
        if ydl is not None:
            def is_supported(url):
                extractors = youtube_dl.extractor.gen_extractors()
                for e in extractors:
                    if e.suitable(url) and e.IE_NAME != 'generic':
                        return True
                return False
            if is_supported(resname):
                info = None
                try:
                    info = ydl.extract_info(resname, download=False)
                except:
                    pass
                if info is not None and 'requested_formats' in info:
                    info = info['requested_formats']
                    resname = info[0]['url']
                    if len(info) > 1:
                        external_audio = info[1]['url']
                elif info is not None and 'formats' in info:
                    resname = info['formats'][-1]['url']
                else:
                    print("unable to parse", resname)
                    return

        if external_audio is None:
            external_audio = ""
        if c_lib.video_play(self.__id, resname.encode(), external_audio.encode(), loop):
            return True
        print("unable to play", resname)
        return False

    #def pause(self):
    #    c_lib.video_pause(self.__id)

    def stop(self):
        c_lib.video_stop(self.__id)

    @property
    def time(self):
        return c_lib.video_get_time(self.__id) * 0.001

    @time.setter
    def time(self, time):
        c_lib.video_set_time(self.__id, int(time * 1000))

    @property
    def volume(self):
        return self.__volume

    @volume.setter
    def volume(self, volume):
        self.__volume = volume
        c_lib.video_set_volume(self.__id, volume)

    @property
    def duration(self):
        return c_lib.video_get_duration(self.__id) * 0.001

    @property
    def texture(self):
        return self.__texture

    def __del__(self):
        c_lib.video_remove(self.__id)
