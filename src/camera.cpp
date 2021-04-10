//

#include "camera.h"
#include "scene.h"

std::vector<camera *> camera::m_update_list;

void camera::update(int dt)
{
    for (auto &c: m_update_list)
        c->m_time += dt;
}

void camera::draw()
{
    for (auto &c: m_update_list)
    {
        if (!c->m_enabled)
            continue;

        if (c->m_update_interval > 0)
        {
            if (c->m_time < c->m_update_interval)
                continue;

            c->m_time %= c->m_update_interval;
        }
        c->render(c->m_texture);
    }
}

int camera::get_origin() const { return m_origin; }
void camera::set_enabled(bool enabled) { m_enabled = enabled; }
void camera::set_fps(int fps) { m_update_interval = fps > 0 ? 1000 / fps : 0; }

void camera::set_fov(float fov)
{
    if (!m_texture)
        return;

    const int w = m_texture->tex->get_width();
    const int h = m_texture->tex->get_height();
    if (!w || !h)
        return;

    m_camera->set_proj(fov, w / float(h), m_znear, m_zfar);
    m_fov = fov;
}

void camera::set_znearfar(float znear, float zfar)
{
    m_znear = znear;
    m_zfar = zfar;
    set_fov(m_fov);
}

void camera::set_texture(int tex_id)
{
    m_fbo[0].release();
    m_fbo[1].release();
    if (tex_id < 0)
        m_texture = 0;

    m_texture = texture::get(tex_id);
    const int w = m_texture->tex->get_width();
    const int h = m_texture->tex->get_height();
    if (!w || !h)
    {
        m_texture = 0;
        return;
    }

    m_textures[0] = m_texture->tex.get();
    m_textures[1].build(0, w, h, m_textures[0].get_format());

    m_fbo[0].set_color_target(m_textures[0].internal().get_shared_data()->tex);
    m_fbo[1].set_color_target(m_textures[1].internal().get_shared_data()->tex);
    m_depth.build_texture(0, w, h, nya_render::texture::depth16);
    m_fbo[0].set_depth_target(m_depth);
    m_fbo[1].set_depth_target(m_depth);
    set_fov(m_fov);
}

void camera::render_to(int tex_id)
{
    scene::instance().update(0);
    render(texture::get(tex_id));
}

void camera::render(texture *t)
{
    const int w = t->tex->get_width();
    const int h = t->tex->get_height();
    if (!w || !h)
        return;

    m_current_idx = 1 - m_current_idx;

    const auto prev_fbo = nya_render::fbo::get_current();
    const auto prev_camera = nya_scene::get_camera_proxy();
    const auto prev_vp = nya_render::get_viewport();

    m_camera->set_pos(m_torigin->get_pos());
    m_camera->set_rot(m_torigin->get_rot());
    nya_scene::set_camera(m_camera);

    nya_render::set_viewport(0, 0, w, h);

    if (t == m_texture)
    {
        m_fbo[m_current_idx].bind();
        scene::instance().draw();
    }
    else
    {
        nya_render::fbo tmp;
        tmp.set_color_target(t->tex->internal().get_shared_data()->tex);
        nya_render::texture tmp_depth;
        tmp_depth.build_texture(0, w, h, nya_render::texture::depth32);
        tmp.set_depth_target(tmp_depth);
        tmp.bind();
        scene::instance().draw();
        tmp.release();
        tmp_depth.release();
    }
    prev_fbo.bind();
    nya_render::set_viewport(prev_vp);
    nya_scene::set_camera(prev_camera);

    m_texture->tex.set(m_textures[m_current_idx]);
}

camera::camera()
{
    m_torigin = transform::get((m_origin = transform::add()));
    m_update_list.push_back(this);
    m_camera.create();
    set_fov(m_fov);
}

camera::~camera()
{
    m_update_list.erase(std::remove(m_update_list.begin(), m_update_list.end(), this), m_update_list.end());
    m_fbo[0].release();
    m_fbo[1].release();
    m_depth.release();
}
