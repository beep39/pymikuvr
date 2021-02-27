//

#include "shape.h"
#include "scene/camera.h"

typedef nya_math::vec3 vec3;

int shape::get_origin() const { return m_origin; }

void shape::set_enabled(bool enabled) { m_enabled = enabled; }

void shape::clear()
{
    m_vertices.clear();
    m_indices2b.clear();
    m_indices4b.clear();
}

void shape::set_sphere(float radius, float s, float t, bool ntc)
{
    clear();

    const int stack_count = 18, sector_count = 36;

    const float sector_step = 2 * nya_math::constants::pi / sector_count;
    const float stack_step = nya_math::constants::pi / stack_count;
    float sector_angle, stack_angle;

    m_vertices.resize((stack_count + 1) * (sector_count + 1));
    auto v = m_vertices.data();

    float xy, z;
    const float length_inv = 1.0f / radius;
    
    float itc_x = 1.0f / (sector_count * s);
    float itc_y = -1.0f / (stack_count * t);

    for(int i = 0; i <= stack_count; ++i)
    {
        stack_angle = nya_math::constants::pi_2 - i * stack_step;
        xy = radius * cosf(stack_angle);
        z = radius * sinf(stack_angle);

        for(int j = 0; j <= sector_count; ++j)
        {
            sector_angle = j * sector_step;

            v->pos.x = xy * sinf(sector_angle);
            v->pos.z = xy * cosf(sector_angle);
            v->pos.y = z;

            v->normal = v->pos * length_inv;

            v->tc.x = j * itc_x;
            v->tc.y = i * itc_y + 1.0f;

            ++v;
        }
    }

    m_indices2b.resize((stack_count * 2 - 2) * sector_count * 3);
    auto* ind = m_indices2b.data();
    int k1, k2;
    for(int i = 0; i < stack_count; ++i)
    {
        k1 = i * (sector_count + 1);
        k2 = k1 + sector_count + 1;

        for(int j = 0; j < sector_count; ++j, ++k1, ++k2)
        {
            if(i != 0)
            {
                *ind++ = k1;
                *ind++ = k2;
                *ind++ = k1 + 1;
            }
            
            if(i != (stack_count-1))
            {
                *ind++ = k1 + 1;
                *ind++ = k2;
                *ind++ = k2 + 1;
            }
        }
    }

    m_type = type_sphere;
    m_size.radius_sq = radius * radius;
    m_aabb_src.origin = nya_math::vec3::zero();
    m_aabb_src.delta = nya_math::vec3(radius, radius, radius);
    m_update = true;
}

void shape::set_cylinder(float radius, float height, float s, float t, bool ntc)
{
    clear();

    const int sector_count = 32;
    const float sector_step = 2 * nya_math::constants::pi / sector_count;

    for(int i = 0; i < 2; ++i)
    {
        const float t = float(i);
        const float h = (t - 0.5f) * height;

        for(int j = 0; j <= sector_count; ++j)
        {
            const float sector_angle = j * sector_step;
            const float x = sin(sector_angle);
            const float z = cos(sector_angle);

            vertex v;
            v.pos.set(x * radius, h, z * radius);
            v.normal.set(x, 0, z);
            v.tc.set((float)j / sector_count, t);
            m_vertices.push_back(v);
        }
    }

    const int base_index = (int)m_vertices.size();
    const int top_index = base_index + sector_count + 1;

    for(int i = 0; i < 2; ++i)
    {
        const float h = (t - 0.5f) * height;
        const float ny = i * 2 - 1;

        vertex v;
        v.pos.y = h;
        v.tc.set(0.5f, 0.5f);
        v.normal.y = ny;
        m_vertices.push_back(v);

        for(int j = 0; j < sector_count; ++j)
        {
            const float sector_angle = j * sector_step;
            const float x = sin(sector_angle);
            const float z = cos(sector_angle);

            vertex v;
            v.pos.set(x * radius, h, z * radius);
            v.normal.set(0, ny, 0);
            v.tc.set(x * 0.5f + 0.5f, z * -0.5 + 0.5);
            m_vertices.push_back(v);
        }
    }

    int k1 = 0, k2 = sector_count + 1;

    if (fabsf(height) > 0.001f)
    {
        for(int i = 0; i < sector_count; ++i, ++k1, ++k2)
        {
            m_indices2b.push_back(k1);
            m_indices2b.push_back(k1 + 1);
            m_indices2b.push_back(k2);

            m_indices2b.push_back(k1 + 1);
            m_indices2b.push_back(k2 + 1);
            m_indices2b.push_back(k2);
        }
    }

    for(int i = 0, k = base_index + 1; i < sector_count; ++i, ++k)
    {
        if(i < sector_count - 1)
        {
            m_indices2b.push_back(base_index);
            m_indices2b.push_back(k + 1);
            m_indices2b.push_back(k);
        }
        else
        {
            m_indices2b.push_back(k);
            m_indices2b.push_back(base_index + 1);
            m_indices2b.push_back(base_index);
        }
    }

    for(int i = 0, k = top_index + 1; i < sector_count; ++i, ++k)
    {
        if(i < sector_count - 1)
        {
            m_indices2b.push_back(top_index);
            m_indices2b.push_back(k);
            m_indices2b.push_back(k + 1);
        }
        else
        {
            m_indices2b.push_back(top_index + 1);
            m_indices2b.push_back(top_index);
            m_indices2b.push_back(k);
        }
    }

    m_type = type_cylinder;
    m_aabb_src.origin = nya_math::vec3::zero();
    m_aabb_src.delta = nya_math::vec3(radius, height * 0.5f, radius);
    m_update = true;
}

void shape::set_box(float x, float y, float z, float s, float t, bool ntc)
{
    clear();

    const bool has_x = fabsf(y * z) > 0.001f;
    const bool has_y = fabsf(x * z) > 0.001f;
    const bool has_z = fabsf(x * y) > 0.001f;

    const int vcount = ((has_x ? 2 : 0) + (has_y ? 2 : 0) + (has_z ? 2 : 0)) * 4;
    m_vertices.resize(vcount);
    auto *v = m_vertices.data();

    if (has_x)
    {
        float ss = 1.0f / s;
        float tt = 1.0f / t;

        if (!ntc)
        {
            ss *= z;
            tt *= y;
        }

        for (int i = 0; i < 2; ++i)
        {
            const float s0 = i * ss, s1 = (1-i) * ss;
            const float xx = (i - 0.5f) * x;
            const vec3 n = vec3(i * 2.0f - 1.0f, 0.0f, 0.0f);
            v->pos.set(xx, y * -0.5f, z * -0.5f);
            v->normal = n;
            v->tc.set(s0, 0);
            ++v;
            v->pos.set(xx, y * 0.5, z * -0.5f);
            v->normal = n;
            v->tc.set(s0, tt);
            ++v;
            v->pos.set(xx, y * 0.5f, z * 0.5f);
            v->normal = n;
            v->tc.set(s1, tt);
            ++v;
            v->pos.set(xx, y * -0.5f, z * 0.5f);
            v->normal = n;
            v->tc.set(s1, 0);
            ++v;
        }
    }
    
    if (has_y)
    {
        float ss = 1.0f / s;
        float tt = 1.0f / t;
        
        if (!ntc)
        {
            ss *= x;
            tt *= z;
        }
        
        for (int i = 0; i < 2; ++i)
        {
            const float s0 = i * ss, s1 = (1-i) * ss;
            const float yy = (i - 0.5f) * y;
            const vec3 n = vec3(0.0f, i * 2.0f - 1.0f, 0.0f);
            v->pos.set(x * -0.5f, yy, z * -0.5f);
            v->normal = n;
            v->tc.set(s1, tt);
            ++v;
            v->pos.set(x * -0.5f, yy, z * 0.5);
            v->normal = n;
            v->tc.set(s1, 0);
            ++v;
            v->pos.set(x * 0.5f, yy, z * 0.5f);
            v->normal = n;
            v->tc.set(s0, 0);
            ++v;
            v->pos.set(x * 0.5f, yy, z * -0.5f);
            v->normal = n;
            v->tc.set(s0, tt);
            ++v;
        }
    }

    if (has_z)
    {
        float ss = 1.0f / s;
        float tt = 1.0f / t;

        if (!ntc)
        {
            ss *= x;
            tt *= y;
        }

        for (int i = 0; i < 2; ++i)
        {
            const float s0 = i * ss, s1 = (1-i) * ss;
            const float zz = (i - 0.5f) * z;
            const vec3 n = vec3(0.0f, 0.0f, i * 2.0f - 1.0f);
            v->pos.set(x * 0.5f, y * -0.5f, zz);
            v->normal = n;
            v->tc.set(s0, 0);
            ++v;
            v->pos.set(x * 0.5f, y * 0.5, zz);
            v->normal = n;
            v->tc.set(s0, tt);
            ++v;
            v->pos.set(x * -0.5f, y * 0.5f, zz);
            v->normal = n;
            v->tc.set(s1, tt);
            ++v;
            v->pos.set(x * -0.5f, y * -0.5f, zz);
            v->normal = n;
            v->tc.set(s1, 0);
            ++v;
        }
    }

    for (int i = 0, voff = 0; voff < vcount; ++i, voff += 4)
    {
        if (i % 2)
        {
            m_indices2b.push_back(voff);
            m_indices2b.push_back(voff + 1);
            m_indices2b.push_back(voff + 2);
            m_indices2b.push_back(voff);
            m_indices2b.push_back(voff + 2);
            m_indices2b.push_back(voff + 3);
        }
        else
        {
            m_indices2b.push_back(voff);
            m_indices2b.push_back(voff + 2);
            m_indices2b.push_back(voff + 1);
            m_indices2b.push_back(voff);
            m_indices2b.push_back(voff + 3);
            m_indices2b.push_back(voff + 2);
        }
    }

    m_type = type_box;
    m_aabb_src.origin = nya_math::vec3::zero();
    m_aabb_src.delta = nya_math::vec3(x, y, z) * 0.5f;
    m_update = true;
}

void shape::set_plane(float w, float h, float s, float t, bool ntc)
{
    clear();

    s = 1.0f / s;
    t = 1.0f / t;

    if (!ntc)
    {
        s *= w;
        t *= h;
    }

    vertex v;
    v.normal = vec3::up();
    v.pos.x = - w * 0.5f;
    v.pos.z = - h * 0.5f;
    v.tc.set(0,t);
    m_vertices.push_back(v);
    v.pos.x = - w * 0.5f;
    v.pos.z = h * 0.5f;
    v.tc.set(0,0);
    m_vertices.push_back(v);
    v.pos.x = w * 0.5f;
    v.pos.z = h * 0.5f;
    v.tc.set(s,0);
    m_vertices.push_back(v);
    v.pos.x = w * 0.5f;
    v.pos.z = - h * 0.5f;
    v.tc.set(s,t);
    m_vertices.push_back(v);

    m_indices2b = {0, 1, 2, 0, 2, 3};

    m_type = type_plane;
    m_aabb_src.origin = nya_math::vec3::zero();
    m_aabb_src.delta = nya_math::vec3(w * 0.5f, 0.0f, h * 0.5f);
    m_update = true;
}

void shape::add_shape(shape *other)
{
    auto voff = (uint32_t)m_vertices.size();

    if (!other->m_indices2b.empty())
        add_tris(other->verts().data(), (uint32_t)other->m_vertices.size(), other->m_indices2b.data(), (uint32_t)other->m_indices2b.size());
    else if (!other->m_indices4b.empty())
        add_tris(other->verts().data(), (uint32_t)other->m_vertices.size(), other->m_indices4b.data(), (uint32_t)other->m_indices4b.size());
    else
        return;

    nya_math::mat4 other_mat(nya_math::quat::invert(m_torigin->get_rot()));
    other_mat.translate(-m_torigin->get_pos()).translate(other->m_torigin->get_pos()).rotate(other->m_torigin->get_rot());

    nya_math::mat4 other_rot_mat(nya_math::quat::invert(m_torigin->get_rot()));
    other_rot_mat.rotate(other->m_torigin->get_rot());

    for (uint32_t i = voff, to = (uint32_t)m_vertices.size(); i < to; ++i)
    {
        m_vertices[i].pos = other_mat * m_vertices[i].pos;
        m_vertices[i].normal = other_rot_mat * m_vertices[i].normal;
    }
    
    m_type = type_triangles;
    calculate_aabb();
    m_update = true;
}

void shape::add_tris(const vertex *verts, uint32_t vcount, const uint16_t *inds, uint32_t icount)
{
    if (!vcount)
        return;

    m_type = type_triangles;
    m_update = true;

    if (!icount)
    {
        std::vector<uint16_t> inds;
        for (uint16_t i = 0; i < (uint16_t)vcount; ++i)
            inds[i] = i;
        add_tris(verts, vcount, inds.data(), vcount);
        calculate_aabb();
        return;
    }

    auto voff = (uint32_t)m_vertices.size();
    m_vertices.resize(voff + vcount);
    memcpy(&m_vertices[voff], verts, vcount * sizeof(vertex));

    //index2b

    if (voff == 0)
    {
        m_indices2b.resize(icount);
        memcpy(m_indices2b.data(), inds, icount * 2);
        calculate_aabb();
        return;
    }

    auto ioff = m_indices2b.size() + m_indices4b.size();
    if (m_indices4b.empty() && m_vertices.size() < 0xffff)
    {
        m_indices2b.resize(ioff + icount);
        for (int i = 0; i < icount; ++i)
            m_indices2b[i + ioff] = inds[i] + voff;
        calculate_aabb();
        return;
    }

    //index4b

    m_indices4b.resize(ioff + icount);
    if (m_vertices.size() >= 0xffff)
    {
        for (int i = 0; i < (int)m_indices2b.size(); ++i)
            m_indices4b[i] = m_indices2b[i];
        m_indices2b.clear();
    }

    for (int i = 0; i < icount; ++i)
        m_indices4b[i + ioff] = inds[i] + voff;
    calculate_aabb();
}

void shape::add_tris(const vertex *verts, uint32_t vcount, const uint32_t *inds, uint32_t icount)
{
    if (!vcount)
        return;

    m_type = type_triangles;
    m_update = true;

    if (!icount)
    {
        std::vector<uint32_t> inds;
        for (uint32_t i = 0; i < (uint32_t)vcount; ++i)
            inds[i] = i;
        add_tris(verts, vcount, inds.data(), vcount);
        calculate_aabb();
        return;
    }

    auto voff = (unsigned int)m_vertices.size();
    m_vertices.resize(voff + vcount);
    memcpy(&m_vertices[voff], verts, vcount * sizeof(vertex));

    auto ioff = m_indices2b.size() + m_indices4b.size();
    m_indices4b.resize(ioff + icount);
    if (!m_indices2b.empty())
    {
        for (int i = 0; i < (int)m_indices2b.size(); ++i)
            m_indices4b[i] = m_indices2b[i];
        m_indices2b.clear();
    }

    for (int i = 0; i < icount; ++i)
        m_indices4b[i + ioff] = inds[i] + voff;
    calculate_aabb();
}

const std::vector<shape::vertex> &shape::verts() { return m_vertices; }
const std::vector<uint16_t> &shape::inds2b() { return m_indices2b; }
const std::vector<uint32_t> &shape::inds4b() { return m_indices4b; }

void shape::set_material(int idx)
{
    if (idx >= 0)
        m_material = material::get(idx);
    else
        m_material = 0;
}

void shape::set_color(float r, float g, float b, float a)
{
    if (m_material)
        m_material->set_param("color", r, g, b, a);
}

void shape::update_post()
{
    m_transform.set_pos(m_torigin->get_pos());
    m_transform.set_rot(m_torigin->get_rot());

    m_aabb = nya_math::aabb(m_aabb_src, m_transform.get_pos(), m_transform.get_rot(), m_transform.get_scale());

    m_zorder = (nya_scene::get_camera().get_pos() - m_transform.get_pos()).length();
}

float shape::zorder() const { return m_zorder; }

void shape::draw(const char *pass)
{
    if(!m_enabled || !m_material)
        return;

    if (m_vertices.empty())
        return;

    if (!nya_scene::get_camera().get_frustum().test_intersect(m_aabb))
        return;

    nya_scene::transform::set(m_transform);
    if (!m_material->set(pass))
        return;

    if (m_update)
    {
        m_mesh.set_vertex_data(m_vertices.data(), (unsigned int)sizeof(vertex), (unsigned int)m_vertices.size());
        if (m_indices4b.empty())
            m_mesh.set_index_data(m_indices2b.data(), nya_render::vbo::index2b, (unsigned int)m_indices2b.size());
        else
            m_mesh.set_index_data(m_indices4b.data(), nya_render::vbo::index4b, (unsigned int)m_indices4b.size());
        m_update = false;
    }

    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material->unset();
}

inline bool tri_intersect(const vec3 &vert0, const vec3 &vert1, const vec3 &vert2, const vec3 &org, const vec3 &dir, float &result)
{
    //The Möller–Trumbore ray-triangle intersection algorithm

    const vec3 e1 = vert1-vert0;
    const vec3 e2 = vert2-vert0;

    const vec3 p = vec3::cross(dir, e2);
    const float det = e1.dot(p);
    const float eps = 0.000001f;

    if(fabsf(det) < eps)
        return 0;

    const float inv_det = 1.f / det;
    const vec3 t = org-vert0;
    const float u = (t.dot(p))*inv_det;
    if(u < 0.0f || u > 1.0f)
        return 0;

    const vec3 q = vec3::cross(t, e1);
    const float v = (dir.dot(q)) * inv_det;
    if(v < 0.0f || u + v  > 1.0f)
        return 0;

    result = (e2.dot(q)) * inv_det;
    return result > eps;
}

bool shape::trace(const vec3 &origin, const vec3 &dir, float &result) const
{
    switch (m_type)
    {
        case type_none: return false;

        case type_sphere:
        {
            const vec3 l = m_torigin->get_pos() - origin;
            const float d = vec3::dot(l, dir);
            const float l_sq = l.length_sq();
            if (d < 0 && l_sq > m_size.radius_sq)
                return false;

            const float m_sq = l_sq - d * d;
            const float rm_sq = m_size.radius_sq - m_sq;
            if (rm_sq < 0)
                return false;

            result = d - sqrt(rm_sq);
            return true;
        }

        //ToDo
        case type_cylinder:
        case type_box:
        case type_plane:

        case type_triangles:
        {
            const auto &r = m_torigin->get_rot();
            const vec3 origin_ = r.rotate_inv(origin - m_torigin->get_pos());
            const vec3 dir_ = r.rotate_inv(vec3::normalize(dir));

            bool hit = false;
            result = INFINITY;

            const vertex *verts = m_vertices.data();
            float len;

            if (m_indices4b.empty())
            {
                const auto *inds = m_indices2b.data();
                const auto *to = inds + m_indices2b.size();

                for (; inds < to; inds += 3)
                if (tri_intersect(verts[ inds[0] ].pos, m_vertices[ inds[1] ].pos, m_vertices[ inds[2] ].pos, origin_, dir_, len))
                {
                    hit = true;
                    if ( result > len)
                        result = len;
                }
            }
            else
            {
                const auto *inds = m_indices4b.data();
                const auto *to = inds + m_indices4b.size();

                for (; inds < to; inds += 3)
                if (tri_intersect(verts[ inds[0] ].pos, m_vertices[ inds[1] ].pos, m_vertices[ inds[2] ].pos, origin_, dir_, len))
                {
                    hit = true;
                    if ( result > len)
                        result = len;
                }
            }
            return hit;
        }
    }

    return false;
}

void shape::calculate_aabb()
{
    if (m_vertices.empty())
    {
        m_aabb_src = nya_math::aabb();
        return;
    }
    
    auto vmin = m_vertices[0].pos;
    auto vmax = m_vertices[0].pos;

    for (auto &v: m_vertices)
    {
        vmin = nya_math::vec3::min(vmin, v.pos);
        vmax = nya_math::vec3::max(vmax, v.pos);
    }

    m_aabb_src = nya_math::aabb(vmin, vmax);
}

shape::shape()
{
    scene::instance().reg_object(this);
    m_torigin = transform::get((m_origin = transform::add()));

    m_mesh.set_vertices(0, 3);
    m_mesh.set_tc(0, 3 * 4, 2);
    m_mesh.set_normals(5 * 4);
}

shape::~shape()
{
    scene::instance().unreg_object(this);
}
