//

#include "sound.h"
#include "player.h"
#include "sys.h"
#include "memory/memory_reader.h"
#include <limits.h>

#include "AL/al.h"
#include "AL/alc.h"

namespace
{
    ALCdevice *m_device = 0;
    ALCcontext *m_context = 0;
}

std::map<std::string, std::shared_ptr<sound::buffer>> sound::m_sound_buffers;
std::vector<std::shared_ptr<sound::source>> sound::m_tmp_sources;
std::vector<sound *> sound::m_update_list;

void sound::init()
{
    m_device = alcOpenDevice(NULL);
    if (!m_device)
        return;

    m_context = alcCreateContext(m_device, NULL);
    if (!m_context)
    {
        alcCloseDevice(m_device);
        return;
    }

    alcMakeContextCurrent(m_context);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alListenerf(AL_GAIN, 1.0f);
}

void sound::update(int dt)
{
    auto h = player::instance().head();
    auto p = h->get_pos();
    alListener3f(AL_POSITION, p.x, p.y, p.z);

    const nya_math::vec3 orientation[] =
    {
        h->get_rot().rotate(nya_math::vec3::forward()),
        h->get_rot().rotate(nya_math::vec3::up())
    };
    alListenerfv(AL_ORIENTATION, &orientation[0].x);

    for (auto &s: m_update_list)
    {
        if (s->m_source && s->m_source->fade_time < s->m_source->fade_goal)
        {
            s->m_source->fade_time += dt;
            alSourcef(s->m_source->sourceid, AL_GAIN, s->m_source->volume * s->m_source->fade_value());
        }
        if (s->m_fadeout_source && s->m_fadeout_source->fade_time < s->m_fadeout_source->fade_goal)
        {
            s->m_fadeout_source->fade_time += dt;
            alSourcef(s->m_fadeout_source->sourceid, AL_GAIN, s->m_fadeout_source->volume * (1.0f - s->m_fadeout_source->fade_value()));

            if (s->m_fadeout_source->fade_time >= s->m_fadeout_source->fade_goal)
            {
                s->m_fadeout_source.reset();
                if (!s->m_source)
                    m_update_list.erase(std::remove(m_update_list.begin(), m_update_list.end(), s), m_update_list.end());
            }
        }

        if (s->m_torigin->version() == s->m_tversion)
            continue;

        const auto &p = s->m_torigin->get_pos();
        if (s->m_source)
            alSource3f(s->m_source->sourceid, AL_POSITION, p.x, p.y, p.z);
        if (s->m_fadeout_source)
            alSource3f(s->m_fadeout_source->sourceid, AL_POSITION, p.x, p.y, p.z);
        s->m_tversion = s->m_torigin->version();
    }
}

void sound::release()
{
    m_tmp_sources.clear();
    m_sound_buffers.clear();

    if (m_context)
    {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(m_context);
        m_context = 0;
    }

    if (m_device)
    {
        alcCloseDevice(m_device);
        m_device = 0;
    }
}

int sound::play(const char *name, bool loop, float fade)
{
    stop(fade);
    m_source = create_src(name, m_volume);
    if (!m_source)
        return 0;

    if (loop)
        alSourcei(m_source->sourceid, AL_LOOPING, true);
    auto &p = m_torigin->get_pos();
    alSource3f(m_source->sourceid, AL_POSITION, p.x, p.y, p.z);
    set_pitch(m_pitch);
    set_radius(m_radius);
    if (std::find(m_update_list.begin(), m_update_list.end(), this) == m_update_list.end())
        m_update_list.push_back(this);
    if (!m_enabled)
        alSourcePause(m_source->sourceid);

    if (fade > 0.01f)
    {
        alSourcef(m_source->sourceid, AL_GAIN, 0.0f);
        m_source->fade_goal = fade * 1000;
        m_source->fade_time = 0;
    }
    return m_source->buffer->length;
}

void sound::stop(float fade)
{
    if (!m_source)
        return;

    if (m_fadeout_source)
        m_fadeout_source.reset();

    if (fade < 0.01f)
    {
        m_source.reset();
        m_update_list.erase(std::remove(m_update_list.begin(), m_update_list.end(), this), m_update_list.end());
        return;
    }

    m_fadeout_source = m_source;
    if (m_fadeout_source->fade_time < m_fadeout_source->fade_goal)
        m_fadeout_source->volume *= m_fadeout_source->fade_value();
    m_fadeout_source->fade_goal = fade * 1000;
    m_fadeout_source->fade_time = 0;
    m_source.reset();
}

void sound::set_volume(float volume)
{
    m_volume = volume;
    if (!m_source)
        return;

    m_source->volume = volume;
    alSourcef(m_source->sourceid, AL_GAIN, volume * m_source->fade_value());
}

void sound::set_pitch(float pitch)
{
    m_pitch = pitch;
    if (m_source)
        alSourcef(m_source->sourceid, AL_PITCH, pitch);
}

void sound::set_radius(float radius)
{
    m_radius = radius;
    if (m_source)
    {
        if (radius < 0)
        {
            alSourcef(m_source->sourceid, AL_REFERENCE_DISTANCE, 0);
            alSourcef(m_source->sourceid, AL_MAX_DISTANCE, 0);
        }
        else
        {
            printf("Unable to set radius on stereo %s\n", m_source->buffer->name.c_str());
            alSourcef(m_source->sourceid, AL_REFERENCE_DISTANCE, radius * 0.5f);
            alSourcef(m_source->sourceid, AL_MAX_DISTANCE, radius);
        }
    }
}

int sound::get_level()
{
    if (!m_source)
        return 0;

    if (m_source->is_finished())
        return 0;

    ALint pos;
    alGetSourcei(m_source->sourceid, AL_BYTE_OFFSET, &pos);
    auto &b = m_source->buffer;
    auto &d = b->data;

    int dt = sys::instance().get_dt();
    if (dt <= 0)
        dt = 1;

    int level = 0;

    switch (b->format)
    {
        case AL_FORMAT_MONO8:
        case AL_FORMAT_STEREO8:
        {
            const size_t sum_len = b->samplerate * (b->format == AL_FORMAT_STEREO8 ? 2 : 1) / dt;
            if (pos + sum_len > d.size())
                return 0;

            const auto *buf = (const char *)(b->data.data() + pos);
            const auto *to = buf + sum_len;
            while (buf < to)
                level = (abs(*buf++) + level) / 2;

            return level * 1000 / SCHAR_MAX;
        }

        case AL_FORMAT_MONO16:
        case AL_FORMAT_STEREO16:
        {
            const size_t sum_len = b->samplerate * (b->format == AL_FORMAT_STEREO16 ? 2 : 1) / dt;
            if (pos + sum_len * sizeof(short) > d.size())
                return 0;

            const auto *buf = (const short *)(b->data.data() + pos);
            const auto *to = buf + sum_len;
            while (buf < to)
                level = (abs(*buf++) + level) / 2;

            return level * 1000 / SHRT_MAX;
        }
    }

    return 0;
}

int sound::get_time()
{
    if (!m_source)
        return 0;

    ALfloat pos;
    alGetSourcef(m_source->sourceid, AL_SEC_OFFSET, &pos);
    return int(pos * 1000);
}

void sound::set_time(int time)
{
    if (m_source)
        alSourcef(m_source->sourceid, AL_SEC_OFFSET, time * 0.001f);
}

int sound::get_origin() const { return m_origin; }

void sound::set_enabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    if (m_source)
    {
        if (enabled)
            alSourcePlay(m_source->sourceid);
        else if (m_source->is_finished())
            m_source.reset();
        else
            alSourcePause(m_source->sourceid);
    }
    m_enabled = enabled;
}

sound::sound() { m_torigin = transform::get((m_origin = transform::add())); m_tversion = m_torigin->version(); }
sound::~sound() { stop(0.0f); }

const void* load_wav(const void* data, size_t data_size, uint32_t& chan, uint32_t& samplerate, uint32_t& bps, uint32_t& size)
{
    nya_memory::memory_reader r(data, data_size);

    struct header
    {
        uint32_t riff;
        uint32_t size;
        uint32_t format;

        uint32_t chunk_id;
        uint32_t chunk_size;

        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t samplerate;
        uint32_t byterate;
        uint16_t block_align;
        uint16_t bps;
    };
    auto h = r.read<header>();
    r.skip(h.chunk_size - 16);

    struct chunk
    {
        uint32_t id;
        uint32_t size;
    };
    
    while (r.check_remained(16))
    {
        auto c = r.read<chunk>();
        if (c.id == 0x61746164)
        {
            chan = h.num_channels;
            samplerate = h.samplerate;
            bps = h.bps;
            size = c.size;
            return r.get_data();
        }
        r.skip(c.size);
    }
    return 0;
}

bool sound::buffer::load(const char *name)
{
    auto r = nya_resources::read_data(name);
    if (!r.get_data())
    {
        printf("resource not found %s\n", name);
        return false;
    }

    uint32_t channels = 0, bps = 0, size = 0;
    const void *data = 0;

    if (r.get_size() > 4 && memcmp(r.get_data(), "RIFF", 4) == 0)
         data = load_wav(r.get_data(), r.get_size(), channels, samplerate, bps, size);
    else
        printf("unsupported sound format in %s\n", name);

    if (!data)
    {
        printf("unable to parse sound format in %s\n", name);
        r.free();
        return false;
    }

    if      (channels == 1 && bps ==  8) format = AL_FORMAT_MONO8;
    else if (channels == 1 && bps == 16) format = AL_FORMAT_MONO16;
    else if (channels == 2 && bps ==  8) format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bps == 16) format = AL_FORMAT_STEREO16;
    else
    {
        if (channels != 1 && channels != 2)
            printf("unsupported channels %d in %s\n", channels, name);
        else
            printf("unsupported bps %d in %s\n", bps, name);
        r.free();
        return false;
    }

    this->name = name;
    alGenBuffers(1, &bufferid);
    alBufferData(bufferid, format, data, size, samplerate);
    this->data.resize(size);
    memcpy(this->data.data(), data, size);
    r.free();

    length = 1000 * size / (samplerate * channels * bps / 8);
    return true;
}

sound::buffer::~buffer()
{
    if (bufferid > 0)
        alDeleteBuffers(1, &bufferid);
}

std::shared_ptr<sound::buffer> sound::get_snd(const char *name)
{
    auto s = m_sound_buffers.find(name);
    if (s != m_sound_buffers.end())
        return s->second;

    auto b = std::make_shared<sound::buffer>();
    if (b->load(name))
    {
        m_sound_buffers[name] = b;
        return b;
    }

    return std::shared_ptr<sound::buffer>();
}

std::shared_ptr<sound::source> sound::create_src(const char *name, float volume)
{
    auto snd = get_snd(name);
    if (!snd)
        return std::shared_ptr<source>();

    auto s = std::make_shared<source>();
    s->buffer = snd;
    ALuint id;
    alGenSources(1, &id);
    alSourcei(id, AL_BUFFER, snd->bufferid);
    alSourcef(id, AL_GAIN, volume);
    alSourcePlay(id);
    s->sourceid = id;
    return s;
}

bool sound::source::is_finished()
{
    ALint state;
    alGetSourcei(sourceid, AL_SOURCE_STATE, &state);
    return state != AL_PLAYING;
}

float sound::source::fade_value()
{
    if (fade_time >= fade_goal)
        return 1.0f;

    return float(fade_time) / fade_goal;
}

sound::source::~source()
{
    if (sourceid > 0)
    {
        alSourceStop(sourceid);
        alDeleteSources(1, &sourceid);
    }
}

void sound::update_tmp_sources()
{
    for (int i = (int)m_tmp_sources.size() - 1; i >= 0; --i)
    {
        if (m_tmp_sources[i]->is_finished())
            m_tmp_sources.erase(m_tmp_sources.begin() + i);
    }
}

bool sound::play2d(const char *name, float volume)
{
    update_tmp_sources();

    auto src = create_src(name, volume);
    if (!src)
        return false;

    alSourcei(src->sourceid, AL_SOURCE_RELATIVE, AL_TRUE);
    m_tmp_sources.push_back(src);
    return true;
}

bool sound::play3d(const char *name, float x, float y, float z, float volume, float pitch, float radius)
{
    update_tmp_sources();

    auto src = create_src(name, volume);
    if (!src)
        return false;

    alSource3f(src->sourceid, AL_POSITION, x, y, z);
    alSourcef(src->sourceid, AL_PITCH, pitch);
    if (radius > 0)
    {
        if (src->buffer->format == AL_FORMAT_STEREO8 || src->buffer->format == AL_FORMAT_STEREO16)
            printf("Unable to set radius on stereo %s\n", name);
        alSourcef(src->sourceid, AL_REFERENCE_DISTANCE, radius * 0.5f);
        alSourcef(src->sourceid, AL_MAX_DISTANCE, radius);
    }
    m_tmp_sources.push_back(src);
    return true;
}

bool sound::preload(const char *name) { return get_snd(name) != 0; }

void sound::set_master_volume(float volume) { alListenerf(AL_GAIN, volume); }
