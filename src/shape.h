//

#pragma once

#include "container.h"
#include "scene.h"
#include "material.h"
#include "texture.h"
#include "transform.h"

class shape: public container<shape>, private scene::iobject
{
public:
    int get_origin() const;
    void set_enabled(bool enabled);

    void clear();
    void set_sphere(float r, float s, float t, bool ntc);
    void set_cylinder(float r, float h, float s, float t, bool ntc);
    void set_box(float x, float y, float z, float s, float t, bool ntc);
    void set_plane(float w, float h, float s, float t, bool ntc);
    void set_heightmap(int count_w, int count_h, float step, const float *heights, float scale, float s, float t, bool ntc);
    void set_heightmap(int count_w, int count_h, float step, const nya_scene::texture_proxy &tex, float scale, float s, float t, bool ntc);

    void add_shape(shape *other);

    void set_material(int mat_id);
    void set_color(float r, float g, float b, float a);

    struct vertex
    {
        nya_math::vec3 pos;
        nya_math::vec2 tc;
        nya_math::vec3 normal;
    };

    void add_tris(const vertex *verts, uint32_t vcount, const uint16_t *inds, uint32_t icount);
    void add_tris(const vertex *verts, uint32_t vcount, const uint32_t *inds, uint32_t icount);

    const std::vector<vertex> &verts();
    const std::vector<uint16_t> &inds2b();
    const std::vector<uint32_t> &inds4b();

    bool trace(const nya_math::vec3 &origin, const nya_math::vec3 &dir, float &result) const;

private:
    void draw(const char *pass);
    void update_pre(int dt) {}
    void update_post();
    float zorder() const { return m_zorder; }
    const nya_math::aabb &get_aabb() const { return m_aabb; }

    void calculate_aabb();

public:
    shape();
    ~shape();

private:
    int m_origin;
    transform *m_torigin;
    bool m_enabled = true;
    nya_scene::transform m_transform;
    float m_zorder = 0;
    
    enum type
    {
        type_none,
        type_sphere,
        type_cylinder,
        type_box,
        type_plane,
        type_heightmap,
        type_triangles,
    };

    type m_type = type_none;

    union
    {
        float radius_sq;
    }
    m_size;

    std::vector<vertex> m_vertices;
    std::vector<uint16_t> m_indices2b;
    std::vector<uint32_t> m_indices4b;
    bool m_update = false;

    nya_render::vbo m_mesh;
    material *m_material = 0;
    nya_scene::texture_proxy m_texture;
    nya_math::aabb m_aabb_src, m_aabb;
};
