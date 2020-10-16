//

#include "camera.h"
#include "phys.h"
#include "player.h"
#include "scene.h"
#include "sound.h"
#include "video.h"
#include "ui.h"

#include "tests/shared/load_pmx.h"
#include "tests/shared/load_pmd.h"
#include "tests/shared/load_xps.h"
#include "tests/shared/load_vmd.h"
#include "tests/shared/mmd_mesh.h"
#include "tests/shared/texture_bgra_bmp.h"
#include "extensions/texture_il.h"

void scene::init()
{
    m_has_context = true;

    nya_render::texture::set_default_aniso(4);
    
    sound::init();
    ui::init();

    m_mmd_phys.init();
    m_mmd_phys.set_gravity(1.0f);

    phys::init();

    nya_scene::mesh::register_load_function(nya_scene::mesh::load_nms, true);
    //nya_scene::mesh::register_load_function(pmd_loader::load, false);
    nya_scene::mesh::register_load_function(pmx_loader::load, false);
    nya_scene::mesh::register_load_function(xps_loader::load_mesh, false);
    nya_scene::mesh::register_load_function(xps_loader::load_mesh_ascii, false);
    nya_scene::texture::register_load_function(nya_scene::load_texture_bgra_bmp, false);
    nya_scene::texture::register_load_function(nya_scene::load_texture_il, false);
    nya_scene::animation::register_load_function(vmd_loader::load, true);
    nya_scene::animation::register_load_function(vmd_loader::load_pose, false);
    nya_scene::animation::register_load_function(nya_scene::animation::load_nan, false);
    nya_scene::animation::register_load_function(xps_loader::load_pose, false);

    m_white.build("\xff\xff\xff\xff", 1, 1, nya_render::texture::color_rgba);
    m_black.build("\xff\x00\x00\x00", 1, 1, nya_render::texture::color_rgba);

    if (!m_pipeline.empty())
        load(m_pipeline.c_str());

    if (m_shadows_resolution > 0)
        set_shadows_resolution(m_shadows_resolution);
    else
        m_shadow_tex.set(m_white);
}

void scene::update(int dt)
{
    //m_dd.clear();

    video::update();
    sound::update();
    phys::update(dt);

    for (auto &o: m_objects)
        o->update_pre(dt);

    m_mmd_phys.update(dt);

    for (auto &o: m_objects)
        o->update_post();

    camera::update(dt);

    std::sort(m_objects.begin(), m_objects.end(), iobject_compare);

    m_update_shadows = m_shadows_enabled;
    m_update_cameras = true;
}

void scene::draw()
{
    auto head = player::instance().head();
    m_camera->set_pos(head->get_pos());
    m_camera->set_rot(head->get_rot());

    if (m_update_shadows)
    {
        auto prev_camera = nya_scene::get_camera_proxy();
        auto vdir = nya_scene::get_camera().get_dir();
        nya_math::vec3 spos = player::instance().origin()->get_pos() + nya_math::vec3(vdir.x, 0, vdir.z).normalize() * m_shadows_size * 0.4;
        m_shadow_camera->set_pos(spos);
        nya_scene::set_camera(m_shadow_camera);
        auto prev_viewport = nya_render::get_viewport();
        auto prev_fbo = nya_render::fbo::get_current();
        nya_render::set_viewport(0, 0, m_shadow_tex->get_width(), m_shadow_tex->get_height());
        m_shadow_fbo.bind();
        nya_render::clear(true, true);
        draw_scene("shadows", nya_scene::tags());
        prev_fbo.bind();
        nya_render::set_viewport(prev_viewport);
        nya_scene::set_camera(prev_camera);
        m_update_shadows = false;
    }

    if (m_update_cameras)
    {
        m_update_cameras = false;
        camera::draw();
    }

    auto cam = nya_scene::get_camera_proxy();
    auto sm = (cam->get_view_matrix() * cam->get_proj_matrix()).invert() *
    (m_shadow_camera->get_view_matrix() * m_shadow_camera->get_proj_matrix());
    for (int j = 0; j < 4; ++j)
    {
        //set_shader_param(("shadow tr"+std::to_string(j)).c_str(), nya_math::vec4(sm[j]));
        m_shadow_tr->set(j, nya_math::vec4(sm[j]));
    }

    postprocess::draw(0);
    phys::debug_draw();
}

void scene::resize(int w, int h)
{
    nya_render::set_viewport(0, 0, w, h);

    m_camera->set_proj(60.0, w / float(h), 0.01f, 300.0f);
}

void scene::release()
{
    unload();
    m_white.unload();
    m_black.unload();
    m_shadow_fbo.release();
    m_shadow_tex.free();
    m_shadow_poisson.free();
    m_has_context = false;
    sound::release();
    ui::release();
    //phys::release();
    //m_mmd_phys.release();
}

void scene::set_light_ambient(float r, float g, float b) { m_light_ambient->set(r, g, b, 1.0f); }
void scene::set_light_color(float r, float g, float b) { m_light_color->set(r, g, b, 1.0f); }
void scene::set_light_dir(float x, float y, float z)
{
    nya_math::vec3 dir(x, y, z);
    dir.normalize();
    m_light_dir->set(-dir.x, -dir.y, -dir.z, 0.0f);
    m_shadow_camera->set_rot(dir);
}

void scene::set_shadows_enabled(bool enabled)
{
    m_shadows_enabled = enabled;
    if (!enabled)
        m_shadow_tex.set(m_white);
}

void scene::set_shadows_resolution(int resolution)
{
    m_shadows_resolution = resolution;
    if (!m_has_context)
        return;

    if (m_shadow_tex->get_width() == resolution && m_shadows_enabled)
        return;

    m_shadows_enabled = true;
    m_shadow_tex.set(nya_scene::texture());
    m_shadow_tex->build(0, resolution, resolution, nya_render::texture::depth32);
    auto stex = m_shadow_tex->internal().get_shared_data()->tex;
    stex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    //m_shadow_fbo.release();
    m_shadow_fbo.set_depth_target(stex);
    set_texture("shadows", m_shadow_tex);

    nya_math::vec3 shadow_poisson[] =
    {
        nya_math::vec3(-0.94201624,  -0.39906216, 0),
        nya_math::vec3( 0.94558609,  -0.76890725, 0),
        nya_math::vec3(-0.094184101, -0.92938870, 0),
        nya_math::vec3(0.34495938,    0.29387760, 0),
        nya_math::vec3(-0.91588581,   0.45771432, 0),
        nya_math::vec3(-0.81544232,  -0.87912464, 0),
        nya_math::vec3(-0.38277543,   0.27676845, 0),
        nya_math::vec3( 0.97484398,   0.75648379, 0),
        nya_math::vec3( 0.44323325,  -0.97511554, 0),
        nya_math::vec3( 0.53742981,  -0.47373420, 0),
        nya_math::vec3(-0.26496911,  -0.41893023, 0),
        nya_math::vec3( 0.79197514,   0.19090188, 0),
        nya_math::vec3(-0.24188840,   0.99706507, 0),
        nya_math::vec3(-0.81409955,   0.91437590, 0),
        nya_math::vec3( 0.19984126,   0.78641367, 0),
        nya_math::vec3( 0.14383161,  -0.14100790, 0)
    };
    for (auto &s: shadow_poisson)
        s *= 3.0f / resolution;
    m_shadow_poisson->build(shadow_poisson, 16, 1, nya_render::texture::color_rgb32f);
    stex = m_shadow_poisson->internal().get_shared_data()->tex;
    stex.set_filter(nya_render::texture::filter_nearest, nya_render::texture::filter_nearest, nya_render::texture::filter_nearest);
}

void scene::set_shadows_size(float size)
{
    m_shadows_size = size;
    const float sh = size * 0.5;
    m_shadow_camera->set_proj(-sh, sh, -sh, sh, -sh * 1.5f, sh);
}

const nya_scene::material::param_proxy &scene::get_light_ambient() const { return m_light_ambient; }
const nya_scene::material::param_proxy &scene::get_light_color() const { return m_light_color; }
const nya_scene::material::param_proxy &scene::get_light_dir() const { return m_light_dir; }
const nya_scene::material::param_array_proxy &scene::get_shadow_tr() const { return m_shadow_tr; }
const nya_scene::texture_proxy &scene::get_shadow_tex() const { return m_shadow_tex; }
const nya_scene::texture_proxy &scene::get_shadow_poisson() const { return m_shadow_poisson; }

bool scene::load_postprocess(const char *name)
{
    if (!m_has_context)
    {
        m_pipeline = name;
        return true;
    }

    return postprocess::load(name);
}

bool scene::get_condition(const char *name)
{
    return postprocess::get_condition(name);
}

void scene::set_condition(const char *name, bool enable)
{
    postprocess::set_condition(name, enable);
}

const nya_scene::texture &scene::white_texture() { return m_white; }
const nya_scene::texture &scene::black_texture() { return m_black; }

btDiscreteDynamicsWorld *scene::mmd_phys()
{
    return m_mmd_phys.get_world();
}

scene::scene()
{
    m_light_ambient.create();
    m_light_color.create();
    m_light_dir.create();

    m_shadow_tr.create();
    m_shadow_tr->set_count(4);
    m_shadow_camera.create();
    m_shadow_tex.create();
    m_shadow_poisson.create();
    
    m_camera = nya_scene::get_camera_proxy();
}

bool scene::check_context()
{
    if (!m_has_context)
    {
        printf("render error: no context\n");
        return false;
    }
    return true;
}

void scene::reg_object(iobject *r)
{
    unreg_object(r);
    m_objects.push_back(r);
}

void scene::unreg_object(const iobject *r)
{
    for (int i = (int)m_objects.size() - 1; i >= 0; --i)
    {
        if (m_objects[i] == r)
        {
            m_objects.erase(m_objects.begin() + i);
            break;
        }
    }
}

void scene::draw_scene(const char *pass,const nya_scene::tags &t)
{
    if (t.has("rsort"))
    {
        for (int i = (int)m_objects.size() - 1; i >= 0; --i)
            m_objects[i]->draw(pass);
    }
    else
    {
        for (auto &r: m_objects)
            r->draw(pass);
    }
}
