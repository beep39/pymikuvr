//

#include "camera.h"
#include "phys.h"
#include "player.h"
#include "scene.h"
#include "sound.h"
#include "video.h"
#include "sys.h"
#include "ui.h"

#include "tests/shared/load_pmx.h"
#include "tests/shared/load_pmd.h"
#include "tests/shared/load_xps.h"
#include "tests/shared/load_vmd.h"
#include "tests/shared/mmd_mesh.h"

#include "render/render_opengl_ext.h"

void scene::init()
{
    m_has_context = true;

    nya_render::texture::set_default_aniso(4);

    sound::init();
    ui::init();

    m_mmd_phys.init();

    phys::init();

    texture::init();

    nya_scene::mesh::register_load_function(nya_scene::mesh::load_nms, true);
    //nya_scene::mesh::register_load_function(pmd_loader::load, false);
    nya_scene::mesh::register_load_function(pmx_loader::load, false);
    nya_scene::mesh::register_load_function(xps_loader::load_mesh, false);
    nya_scene::mesh::register_load_function(xps_loader::load_mesh_ascii, false);
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
    m_shadow_update_skip = (m_shadow_update_skip + 1) % 4;
    m_update_cameras = true;
}

bool scene::set_shadow_proj(const nya_math::mat4 &view, float dist_near, float dist_far)
{
    const nya_math::mat4 &shadow_view = m_shadow_camera->get_view_matrix();

    nya_math::vec3 vmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    nya_math::vec3 vmax = -vmin;

    for (const nya_math::mat4 &proj: m_proj)
    {
        const float proj32 = proj[3][2], proj22 = proj[2][2], proj33 = proj[3][3], proj23 = proj[2][3];
        const float pnear = std::max((proj32 - proj22 * dist_near) / (proj33 - proj23 * dist_near), -1.0f);
        const float pfar  = std::min((proj32 - proj22 * dist_far)  / (proj33 - proj23 * dist_far),   1.0f);

        const nya_math::vec4 frustum_corners[] =
        {
            nya_math::vec4( 1.0f,  1.0f, pnear, 1.0f),
            nya_math::vec4(-1.0f,  1.0f, pnear, 1.0f),
            nya_math::vec4( 1.0f, -1.0f, pnear, 1.0f),
            nya_math::vec4(-1.0f, -1.0f, pnear, 1.0f),

            nya_math::vec4( 1.0f,  1.0f, pfar, 1.0f),
            nya_math::vec4(-1.0f,  1.0f, pfar, 1.0f),
            nya_math::vec4( 1.0f, -1.0f, pfar, 1.0f),
            nya_math::vec4(-1.0f, -1.0f, pfar, 1.0f),
        };

        const nya_math::mat4 m = (view * proj).invert() * shadow_view;
        for (const nya_math::vec4 &c: frustum_corners)
        {
            auto v = m * c;
            v.xyz() /= v.w;
            vmin = nya_math::vec3::min(vmin, v.xyz());
            vmax = nya_math::vec3::max(vmax, v.xyz());
        }
    }

    //ToDo
    auto sproj = nya_math::mat4().ortho(vmin.x, vmax.x, vmin.y, vmax.y, -1024.0f, 1024.0f);
    const auto frustum = nya_math::frustum(shadow_view * sproj);
    float zmin = std::numeric_limits<float>::max();
    float zmax = -zmin;
    for (auto &r: m_objects)
    {
        const auto &aabb = r->get_aabb();
        if (!frustum.test_intersect(aabb))
            continue;

        const auto v = shadow_view * aabb.origin;
        const float d = aabb.delta.length();
        zmax = nya_math::max(zmax, v.z + d);
        zmin = nya_math::min(zmin, v.z - d);
    }

    if (zmin >= zmax) //No objects inside shadow frustum
        return false;

    m_shadow_camera->set_proj(nya_math::mat4().ortho(vmin.x, vmax.x, vmin.y, vmax.y, zmin, zmax));
    return true;
}

void scene::draw()
{
    auto head = player::instance().head();
    m_camera->set_pos(head->get_pos());
    m_camera->set_rot(head->get_rot());

    if (m_update_shadows)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);

        auto prev_camera = nya_scene::get_camera_proxy();
        auto prev_viewport = nya_render::get_viewport();
        auto prev_fbo = nya_render::fbo::get_current();

        const auto view_matrix = prev_camera->get_view_matrix();

        m_shadow_fbo.bind();
        nya_render::set_viewport(0, 0, m_shadow_tex->get_width(), m_shadow_tex->get_height());
        nya_scene::set_camera(m_shadow_camera);

        float prevz = 0.0f;
        const float width = m_shadow_tex->get_width() * 0.5f;
        const float height = m_shadow_tex->get_height() * 0.5f;
        const static nya_math::vec2 shadow_offsets[] = { nya_math::vec2(), nya_math::vec2(1.0f, 0.0f),
                                                         nya_math::vec2(0.0f, 1.0f), nya_math::vec2(1.0f, 1.0f) };
        for (int i = 0; i < 4; ++i)
        {
            const float dist = m_shadow_cascades_dist[i];
            if (dist <= 0)
                break;

            //since vr runs at 90-144 fps, not all cascades are updated every frame
            if (m_shadow_cascades_dist[3] > 0)
            {
                if (m_shadow_update_skip != 1 && i == 2)
                    continue;
                if (m_shadow_update_skip != 3 && i == 3)
                    continue;
            }
            if (m_shadow_update_skip % 2)
            {
                if (i == 1)
                    continue;
            }
            else if (i == 2)
                continue;

            const bool draw = set_shadow_proj(view_matrix, prevz, dist);
            prevz = dist;

            nya_render::rect r;

            if (m_shadow_cascades_dist[1] > 0)
            {
                const auto &off = shadow_offsets[i];
                const nya_math::vec3 moff(-1.0f + off.x * 2.0f, -1.0f + off.y * 2.0f, 0.0f);
                m_shadow_matrices[i] = m_shadow_camera->get_proj_matrix() * nya_math::mat4().scale(0.5f, 0.5f, 1.0f).translate(moff);
                r.x = off.x * width;
                r.y = off.y * height;
                r.width = width;
                r.height = height;
            }
            else
            {
                m_shadow_matrices[i] = m_shadow_camera->get_proj_matrix();
                r.width = width * 2.0f;
                r.height = height * 2.0f;
            }
            
            nya_render::scissor::enable(r);
            nya_render::clear(false, true);

            if (!draw)
                continue;

            glPolygonOffset(m_shadow_cascades_bias[i], m_shadow_cascades_slope_bias[i]);

            m_shadow_camera->set_proj(m_shadow_matrices[i]);
            draw_scene("shadows", nya_scene::tags());
        }
        nya_render::scissor::disable();
        prev_fbo.bind();
        nya_render::set_viewport(prev_viewport);
        nya_scene::set_camera(prev_camera);
        m_update_shadows = false;

        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    if (m_update_cameras)
    {
        m_update_cameras = false;
        camera::draw();
    }

    auto cam = nya_scene::get_camera_proxy();

    static const auto bias = nya_math::mat4().scale(0.5).translate(1.0,1.0,1.0);
    const auto imvpshv = (cam->get_view_matrix() * cam->get_proj_matrix()).invert() * m_shadow_camera->get_view_matrix();
    nya_math::mat4 matrices[4];
    for (int i = 0; i < 4; ++i)
        matrices[i] = imvpshv * m_shadow_matrices[i] * bias;
    memcpy(m_shadow_tr->get_buf(), matrices, sizeof(matrices));

    postprocess::draw(0);
    phys::debug_draw();
}

void scene::resize(int w, int h)
{
    nya_render::set_viewport(0, 0, w, h);

    m_camera->set_proj(60.0, w / float(h), m_znear, m_zfar);
    m_proj = { m_camera->get_proj_matrix() };
}

void scene::release()
{
    unload();
    m_white.unload();
    m_black.unload();
    m_shadow_fbo.release();
    m_shadow_tex.free();
    m_has_context = false;
    sound::release();
    ui::release();
    //phys::release();
    //m_mmd_phys.release();
}

void scene::set_proj(const nya_math::mat4 &left, const nya_math::mat4 &right) { m_proj = {left, right}; }

void scene::set_znearfar(float znear, float zfar)
{
    m_znear = znear;
    m_zfar = zfar;
    sys::instance().set_znearfar(znear, zfar);
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
    m_shadow_tex->build(0, resolution, resolution, nya_render::texture::depth16);
    auto stex = m_shadow_tex->internal().get_shared_data()->tex;
    stex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    //m_shadow_fbo.release();
    m_shadow_fbo.set_depth_target(stex);
    set_texture("shadows", m_shadow_tex);
}

void scene::set_shadows_cascades(float c0, float c1, float c2, float c3)
{
    m_shadow_cascades_dist[0] = c0;
    m_shadow_cascades_dist[1] = c1;
    m_shadow_cascades_dist[2] = c2;
    m_shadow_cascades_dist[3] = c3;

    const float max_dist = 1000000.0f;
    m_shadow_cascades->set(c0 <= 0 ? max_dist : c0, c1 <= 0 ? max_dist : c1, c2 <= 0 ? max_dist : c2, c3 <= 0 ? max_dist : c3);
}

void scene::set_shadows_bias(int idx, float bias, float slope)
{
    if (idx < 0 || idx >= 4)
        return;

    m_shadow_cascades_bias[idx] = bias;
    m_shadow_cascades_slope_bias[idx] = slope;
}

const nya_scene::material::param_proxy &scene::get_light_ambient() const { return m_light_ambient; }
const nya_scene::material::param_proxy &scene::get_light_color() const { return m_light_color; }
const nya_scene::material::param_proxy &scene::get_light_dir() const { return m_light_dir; }
const nya_scene::material::param_array_proxy &scene::get_shadow_tr() const { return m_shadow_tr; }
const nya_scene::material::param_proxy &scene::get_shadow_cascades() const { return m_shadow_cascades; }
const nya_scene::texture_proxy &scene::get_shadow_tex() const { return m_shadow_tex; }

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
    m_shadow_tr->set_count(4 * 4);
    m_shadow_cascades.create();
    m_shadow_camera.create();
    m_shadow_tex.create();
    
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
