//

#include "texture.h"
#include "scene.h"
#include "render/bitmap.h"
#include "tests/shared/texture_bgra_bmp.h"

#ifndef _WIN32
    #include "extensions/texture_il.h"
#else
namespace Gdiplus {  using std::min; using std::max; }
#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

namespace
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
}

static bool load_gdi_plus(nya_scene::shared_texture &res,nya_scene::resource_data &data,const char* name)
{
    if(!data.get_size())
        return false;

    IStream *stream = SHCreateMemStream((BYTE *)data.get_data(), data.get_size());
    Gdiplus::Bitmap image(stream);
    stream->Release();

    const int w = (int)image.GetWidth();
    const int h = (int)image.GetHeight();
    Gdiplus::Rect rc(0, 0, w, h);
    Gdiplus::BitmapData bitmap_data;
    if (image.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmap_data) != Gdiplus::Ok)
        return false;

    nya_memory::tmp_buffer_scoped buf(w*h*4);
    auto col = (uint32_t *)buf.get_data();
    for (int y = 0, stride = w * 4; y < h; ++y, col += w)
        memcpy(col, (char *)bitmap_data.Scan0 + bitmap_data.Stride * (h - 1 - y), stride);
    image.UnlockBits(&bitmap_data);

    nya_render::bitmap_argb_to_rgba((uint8_t *)buf.get_data(), w, h);

    if (nya_render::bitmap_is_full_alpha((uint8_t *)buf.get_data(), w, h))
    {
        nya_render::bitmap_rgba_to_rgb((uint8_t *)buf.get_data(), w, h);
        return res.tex.build_texture(buf.get_data(), w, h, nya_render::texture::color_rgb);
    }

    return res.tex.build_texture(buf.get_data(), w, h, nya_render::texture::color_rgba);
}
#endif

void texture::init()
{
    nya_scene::texture::register_load_function(nya_scene::load_texture_bgra_bmp, false);
#ifdef _WIN32
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    nya_scene::texture::register_load_function(load_gdi_plus, false);
#else
    nya_scene::texture::register_load_function(nya_scene::load_texture_il, false);
#endif
}

void texture::make_white(int id) { get(id)->tex = nya_scene::texture_proxy(scene::instance().white_texture()); }
void texture::make_black(int id) { get(id)->tex = nya_scene::texture_proxy(scene::instance().black_texture()); }
void texture::make_normal(int id) { get(id)->tex = nya_scene::texture_proxy(scene::instance().normal_texture()); }

bool texture::save(const char *name)
{
#ifdef _WIN32
    return false;
#else
    return nya_scene::save_texture_il(tex.get(), name);
#endif
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
