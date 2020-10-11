//

#include "material.h"
#include "scene.h"
#include "texture.h"

void material::copy(int from, int to)
{
    get(to)->mat = get(from)->mat;
}

bool material::load(const char *name)
{
    bool result = mat.load(name);
    if (result)
    {
        mat.set_texture("diffuse", scene::instance().white_texture());
        mat.set_param("light ambient", scene::instance().get_light_ambient());
        mat.set_param("light color", scene::instance().get_light_color());
        mat.set_param("light dir", scene::instance().get_light_dir());
        mat.set_param_array("shadow tr", scene::instance().get_shadow_tr());
        mat.set_texture("shadow", scene::instance().get_shadow_tex());
        mat.set_texture("shadow poisson", scene::instance().get_shadow_poisson());
        auto c = mat.get_param("color");
        m_alpha = c.is_valid() ? c->w : 1.0f;
    }
    else
        m_alpha = 0.0f;
    update_opaque();
    return result;
}

void material::set_texture(const char *type, int idx)
{
    if (idx >= 0)
    {
        auto *t = texture::get(idx);
        mat.set_texture(type, t->tex);
    }
    else
        mat.set_texture(type, scene::instance().white_texture());

    if (strcmp(type, "diffuse") == 0)
        update_opaque();
}

void material::set_param(const char *type, float r, float g, float b, float a)
{
    mat.set_param(type, r, g, b, a);
    if (strcmp(type, "color") == 0)
    {
        m_alpha = a;
        update_opaque();
    }
}

void material::update_opaque()
{
    m_zero_alpha = m_alpha < 0.00001f;
    m_alpha_clip = m_alpha >= 0.89999f;
    m_opaque = m_alpha >= 0.99999f;

    if (m_opaque)
    {
        auto t = mat.get_texture("diffuse");
        if (t.is_valid())
        {
            const auto f = t->get_format();
            if (f == nya_render::texture::color_bgra || f == nya_render::texture::color_rgba)
            {
                m_opaque = false;
                m_alpha_clip = true;
            }
        }
    }

    if (m_opaque && mat.get_pass_idx("opaque") < 0)
        m_opaque = false;
}

bool material::set(const char *pass)
{
    if (m_zero_alpha)
        return false;

    if (m_opaque)
    {
        static const char tr[] = "transparent";
        if (strncmp(pass, tr, sizeof(tr) - 1) == 0)
            return false;
    }
    else
    {
        if (strcmp(pass, "opaque") == 0)
            return false;

        if (!m_alpha_clip && strcmp(pass, "transparent_clip") == 0)
            return false;
    }

    mat.internal().set(pass);
    return true;
}

void material::unset()
{
    mat.internal().unset();
}
