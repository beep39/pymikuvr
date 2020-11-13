//

#include "video.h"
#include "sys.h"
#include "render/texture.h"
#include "render/bitmap.h"
#include "system/system.h"

#if defined(_MSC_VER)
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif
#include "vlc/vlc.h"

std::vector<video *> video::m_update_list;

void video::update()
{
    for (auto &v: m_update_list)
    {
        if(!v->m_player)
            continue;

        if(v->m_mutex.try_lock())
        {
            if(v->m_need_update)
            {
                if (v->m_length == 0)
                    v->m_length = (int)libvlc_media_player_get_length(v->m_player);
                v->swap_chain();
                v->m_texture->build(v->m_pixel_buffer.data(), v->m_width, v->m_height, nya_render::texture::color_rgb);
                v->m_need_update = false;
            }
            v->m_mutex.unlock();
        }

        auto global_time = nya_system::get_time();
        libvlc_time_t vlc_time = libvlc_media_player_get_time(v->m_player);
        if (vlc_time != v->m_prev_vlc_time)
        {
            v->m_time = int(vlc_time * 0.001f);
            v->m_prev_vlc_time = vlc_time;
        }
        else
            v->m_time += int(global_time - v->m_global_time);
        v->m_global_time = global_time;
    }
}

void video::set_texture(int texture)
{
    m_texture = texture::get(texture)->tex;
    m_texture->build("\x00\x00\x00", 1, 1, nya_render::texture::color_rgb);
    m_texture_swap.create();
}

inline bool starts_with(const std::string &str, const std::string &search)
{
    return search.length() <= str.length() && equal(search.begin(), search.end(), str.begin());
}

static bool is_local(std::string path) { return !starts_with(path, "http://") && !starts_with(path, "https://"); }

static std::string fix_path(const char *path)
{
    if (!path || !path[0])
        return std::string();

    const auto url = std::string(path);
    return is_local(url) ? sys::instance().get_path(path) : url;
}

bool video::play(const char *path, const char *audio_path, bool loop)
{
    stop();

    auto url = fix_path(path);
    auto url_a = fix_path(audio_path);

    if (url.empty())
    {
        if (!url_a.empty())
        {
            url = url_a;
            url_a.clear();
        }
        else
            return false;
    }

    const char *argv[] = {/* "-vvv", */"--no-xlib", "--no-video-title-show", "--input-repeat=65545" };
    m_vlc = libvlc_new(sizeof(argv) / sizeof(*argv) - (loop ? 0 : 1), argv);
    if (!m_vlc)
    {
        printf("unable to initialize libvlc\n");
        return false;
    }

    libvlc_media_t *m = is_local(url) ? libvlc_media_new_path(m_vlc, url.c_str())
                                      : libvlc_media_new_location(m_vlc, url.c_str());
    if (!m)
    {
        printf("unable to load video %s\n", url.c_str());
        libvlc_release(m_vlc);
        return false;
    }

    if (!url_a.empty())
        libvlc_media_slaves_add(m, libvlc_media_slave_type_audio, 0, url_a.c_str());

    m_player = libvlc_media_player_new_from_media(m);
    libvlc_media_release(m);
    set_volume(m_volume);

    libvlc_video_set_callbacks(m_player, video_lock_callback, video_unlock_callback, video_display_callback, this);
    libvlc_video_set_format_callbacks(m_player, format_callback, format_cleanup);
    m_time = 0;
    m_prev_vlc_time = 0;

    libvlc_media_player_play(m_player);
    m_update_list.push_back(this);
    return true;
}

void video::stop()
{
    if(!m_player)
        return;

    libvlc_media_player_stop(m_player);
    libvlc_media_player_release(m_player);
    m_player = 0;
    libvlc_release(m_vlc);
    m_vlc = 0;
    m_length = 0;
    m_update_list.erase(std::remove(m_update_list.begin(), m_update_list.end(), this), m_update_list.end());
}

void video::pause()
{
    //ToDo
}

void video::set_volume(float volume)
{
    if (m_player)
    {
        const float v = nya_math::clamp(powf(volume, 0.333f), 0.0f, 1.0f);
        libvlc_audio_set_volume(m_player, int(v * 100));
    }
    m_volume = volume;
}

int video::get_time() { return m_time; }
int video::get_length() { return m_length; }

void video::set_time(int time)
{
    if (m_player)
        libvlc_media_player_set_time(m_player, time);
}

video::~video() { stop(); }

void *video::video_lock_callback(void *object, void **planes)
{
    video* self = (video *)object;
    self->m_mutex.lock();
    planes[0] = (void *)self->m_pixel_buffer.data();
    return NULL;
}

void video::video_unlock_callback(void *object, void *picture, void * const *planes)
{
    video* self = (video *)object;
    nya_render::bitmap_flip_vertical(self->m_pixel_buffer.data(), self->m_width, self->m_height, 3);
    self->m_need_update= true;
    self->m_mutex.unlock();
}

void video::video_display_callback(void *object, void *picture) {}

unsigned int video::format_callback(void **object, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
    video* self = (video *)*object;
    self->m_mutex.lock();
    self->m_width = *width;
    self->m_height = *height;
    self->m_pixel_buffer.resize((*width) * (*height) * 3);
    self->m_mutex.unlock();

    chroma[0] = 'R';
    chroma[1] = 'V';
    chroma[2] = '2';
    chroma[3] = '4';
    *pitches = *width * 3;
    *lines = *height;
    return 1;
}

void video::format_cleanup(void *object) {}
