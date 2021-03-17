//

#pragma once

#include "singleton.h"
#include "render/debug_draw.h"
#include "scene/postprocess.h"
#include "scene/camera.h"
#include "scene/material.h"
#include "tests/shared/mmd_phys.h"

class scene: public singleton<scene>, private nya_scene::postprocess
{
public:
    void init();
    void update(int dt);
    void draw();
    void resize(int w, int h);
    void release();

    void set_proj(const nya_math::mat4 &left, const nya_math::mat4 &right);
    
    void set_znearfar(float znear, float zfar);

    void set_light_ambient(float r, float g, float b);
    void set_light_color(float r, float g, float b);
    void set_light_dir(float x, float y, float z);

    void set_shadows_enabled(bool enabled);
    void set_shadows_resolution(int resolution);
    void set_shadows_cascades(float c0, float c1, float c2, float c3);
    void set_shadows_bias(int idx, float bias, float slope);

    const nya_scene::material::param_proxy &get_light_ambient() const;
    const nya_scene::material::param_proxy &get_light_color() const;
    const nya_scene::material::param_proxy &get_light_dir() const;
    const nya_scene::material::param_array_proxy &get_shadow_tr() const;
    const nya_scene::material::param_proxy &get_shadow_cascades() const;
    const nya_scene::texture_proxy &get_shadow_tex() const;

    bool load_postprocess(const char *name);
    bool get_condition(const char *name);
    void set_condition(const char *name, bool enable);

    const nya_scene::texture &white_texture();
    const nya_scene::texture &black_texture();

    btDiscreteDynamicsWorld *mmd_phys();

    scene();

    class iobject
    {
    public:
        virtual void update_pre(int dt) = 0;
        virtual void update_post() = 0;
        virtual void draw(const char *pass) = 0;
        virtual float zorder() const = 0;
        virtual const nya_math::aabb &get_aabb() const = 0;
    };

    static inline bool iobject_compare(const iobject *a, const iobject *b) { return a->zorder() < b->zorder(); };

    void reg_object(iobject *r);
    void unreg_object(const iobject *r);

private:
    bool set_shadow_proj(const nya_math::mat4 &view, float near, float far);
    bool check_context();
    void draw_scene(const char *pass, const nya_scene::tags &t) override;

private:
    nya_scene::material::param_proxy m_light_ambient, m_light_color, m_light_dir;
    nya_scene::camera_proxy m_shadow_camera;
    nya_scene::material::param_array_proxy m_shadow_tr;
    nya_scene::material::param_proxy m_shadow_cascades;
    nya_scene::texture_proxy m_shadow_tex;
    nya_render::fbo m_shadow_fbo;
    std::vector<nya_math::mat4> m_proj;
    float m_znear = 0.1, m_zfar = 300;
    float m_shadow_cascades_dist[4] = {10.0f, -1.0f, -1.0f, -1.0f};
    float m_shadow_cascades_bias[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float m_shadow_cascades_slope_bias[4] = {7.0f, 7.0f, 7.0f, 7.0f};
    int m_shadow_update_skip = 0;
    nya_math::mat4 m_shadow_matrices[4];

    bool m_has_context = false;
    std::vector<iobject *> m_objects;
    bool m_shadows_enabled = false;
    bool m_update_shadows = false;
    bool m_update_cameras = false;
    int m_shadows_resolution = 0;
    std::string m_pipeline;

    nya_scene::texture m_white;
    nya_scene::texture m_black;

    mmd_phys_world m_mmd_phys;
    nya_scene::camera_proxy m_camera;
};
