//

#pragma once

#include "texture.h"
#include <mutex>

struct libvlc_instance_t;
struct libvlc_media_player_t;

class video: public container<video>
{
public:
    static void update();

public:
    bool play(const char *path, const char *audio_path, bool loop);
    void pause();
    void stop();
    void set_volume(float volume);
    int get_time();
    int get_length();
    void set_time(int time);

    void set_texture(int texture);

    ~video();

private:
    static void *video_lock_callback(void *object, void **planes);
    static void video_unlock_callback(void *object, void *picture, void * const *planes);
    static void video_display_callback(void *object, void *picture);
    static unsigned int format_callback(void **object, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines);
    static void format_cleanup(void *object);

private:
    void swap_chain()
    {
        m_texture_chain_idx = (m_texture_chain_idx + 1) % (sizeof(m_texture_chain) / sizeof(m_texture_chain[0]));
        m_texture.set(m_texture_chain[m_texture_chain_idx]);
    }
    int m_texture_chain_idx = 0;
    nya_scene::texture m_texture_chain[2];
    nya_scene::texture_proxy m_texture;

    float m_volume = 1.0f;
    int m_length = 0;
    int m_width = 0, m_height = 0;

    bool m_need_update = false;
    std::vector<unsigned char> m_pixel_buffer;
    libvlc_instance_t *m_vlc = 0;
    libvlc_media_player_t *m_player = 0;
    int m_time = 0;
    int64_t m_prev_vlc_time = 0;
    uint64_t m_global_time = 0;
    std::mutex m_mutex;

    static std::vector<video *> m_update_list;
};
