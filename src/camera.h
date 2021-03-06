//

#pragma once

#include "texture.h"
#include "transform.h"
#include "scene/camera.h"
#include "render/fbo.h"

class camera: public container<camera>
{
public:
    static void update(int dt);
    static void draw();

    int get_origin() const;
    void set_enabled(bool enabled);
    void set_fps(int fps);
    void set_fov(float fov);
    void set_znearfar(float znear, float zfar);
    void set_texture(int tex_id);
    void render_to(int tex_id);

    camera();
    ~camera();

private:
    void render(texture *t);

    int m_origin;
    transform *m_torigin;
    unsigned int m_tversion;
    bool m_enabled = true;
    unsigned int m_update_interval = 0;
    unsigned int m_time = 0;
    float m_fov = 60.0f;
    float m_znear = 0.1, m_zfar = 300;

    nya_scene::camera_proxy m_camera;
    texture *m_texture = 0;
    
    nya_render::fbo m_fbo[2];
    nya_scene::texture m_textures[2];
    int m_current_idx = 0;
    nya_render::texture m_depth;

    static std::vector<camera *> m_update_list;
};
