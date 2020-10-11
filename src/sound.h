//

#pragma once

#include "container.h"
#include "transform.h"
#include <map>
#include <string>

class sound: public container<sound>
{
public:
    static void init();
    static void update();
    static void release();

public:
    static bool play2d(const char *name, float volume);
    static bool play3d(const char *name, float x, float y, float z, float volume, float pitch, float radius);
    static bool preload(const char *name);

public:
    int play(const char *name, bool loop);
    void stop();
    void set_radius(float radius);
    void set_volume(float volume);
    void set_pitch(float pitch);
    int get_time();
    void set_time(int time);
    int get_level();
    int get_origin() const;
    void set_enabled(bool enabled);

    sound();
    ~sound();

private:
    struct buffer
    {
        std::string name;
        unsigned int bufferid = 0, length = 0, format = 0, samplerate = 0;
        std::vector<char> data;

        bool load(const char *name);
        ~buffer();
    };
    static std::map<std::string, std::shared_ptr<buffer>> m_sound_buffers;

    static std::shared_ptr<buffer> get_snd(const char *name);

    struct source
    {
        unsigned int sourceid = 0;
        std::shared_ptr<buffer> buffer;

        bool is_finished();
        ~source();
    };
    static std::vector<std::shared_ptr<source>> m_tmp_sources;
    static std::vector<sound *> m_update_list;
    static std::shared_ptr<source> create_src(const char *name, float volume);

    static void update_tmp_sources();

private:
    int m_origin;
    transform *m_torigin;
    unsigned int m_tversion;
    bool m_enabled = true;
    std::shared_ptr<source> m_source;
    float m_volume = 1.0f;
    float m_pitch = 1.0f;
    float m_radius = -1.0f;
};
