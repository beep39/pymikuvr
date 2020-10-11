//

#include "texture.h"
#include "extensions/texture_il.h"

bool texture::save(const char *name)
{
    return nya_scene::save_texture_il(tex.get(), name);
}

void texture::build(int w, int h, float r, float g, float b, float a)
{
    uint32_t data = (uint8_t(a * 0xff) << 24) + (uint8_t(b * 0xff) << 16) + (uint8_t(g * 0xff) << 8) + uint8_t(r * 0xff);

    nya_memory::tmp_buffer_scoped buf(w * h * 4);
    auto c = (uint32_t *)buf.get_data();
    auto to = c + w * h;
    while(c < to)
        *c++ = data;
    tex->build(buf.get_data(), w, h, nya_render::texture::color_rgba);
}

void texture::copy(int from, int to)
{
    get(to)->tex.set(get(from)->tex.get());
}
