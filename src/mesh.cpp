//

#include "mesh.h"
#include "animation.h"
#include "shape.h"
#include "scene/camera.h"
#include "tests/shared/load_pmx.h"
#include "tests/shared/load_pmd.h"

inline float mmd_scale()
{
    const float miku_height = 1.58f;
    const float miku_model_height = 19.75f;
    return miku_height / miku_model_height;
}

static bool load_relative(const char *name, nya_scene::mesh &m)
{
    std::string path(name);
    const size_t s = path.rfind("/");
    path.resize(s == std::string::npos ? 0 : s + 1);

    const std::string mat_prefix = nya_scene::material_internal::get_resources_prefix();
    const std::string sh_prefix = nya_scene::shader_internal::get_resources_prefix();
    const std::string tex_prefix = nya_scene::texture_internal::get_resources_prefix();

    nya_scene::material::set_resources_prefix(path.c_str());
    nya_scene::shader::set_resources_prefix(path.c_str());
    nya_scene::texture::set_resources_prefix(path.c_str());

    const bool result = m.load(name);

    nya_scene::material::set_resources_prefix(mat_prefix.c_str());
    nya_scene::shader::set_resources_prefix(sh_prefix.c_str());
    nya_scene::texture::set_resources_prefix(tex_prefix.c_str());

    return result;
}

bool mesh::load(const char *name)
{
    m_phys.release();
    m_bones.clear();
    m_groups_visible.clear();
    m_groups_count = 0;
    m_first_update = true;
    
    const std::string mat_prefix = nya_scene::material_internal::get_resources_prefix();

    bool result;
    if (nya_resources::check_extension(name, "nms"))
        result = load_relative(name, m_mesh);
    else
    {
        nya_scene::material::set_resources_prefix("materials/");
        result = m_mesh.load(name);
    }

    m_groups_count = m_mesh.get_groups_count();

    if (m_mesh.is_mmd())
    {
        const auto *pmx = pmx_loader::get_additional_data(m_mesh);
        if (pmx)
            m_groups_count = (int)pmx->materials.size();

        for (int i = 0; i < m_mesh.get_groups_count(); ++i)
        {
            const auto &om = m_mesh.get_material(i);
            const auto &op = om.get_pass("opaque");
            const auto &os = op.get_state();

            const bool edge = i >= m_groups_count;
            bool opaque = true;
            if (!edge)
                opaque = om.get_param("diff k")->w > 0.999f;

            auto &ot = om.get_texture("diffuse");
            if (ot.is_valid())
            {
                auto f = ot->get_format();
                if (f == nya_render::texture::color_bgra || f == nya_render::texture::color_rgba)
                    opaque = false;
            }

            //ToDo: material morphs disables opaque

            auto m = nya_scene::material(edge ? "mmd_edge.txt" : (opaque ? "mmd.txt" : "mmd_a.txt"));
            for (int j = 0; j < om.get_textures_count(); ++j)
                m.set_texture(om.get_texture_semantics(j), om.get_texture(j));
            for (int j = 0; j < om.get_params_count(); ++j)
            {
                auto &p = om.get_param(j);
                if (p.is_valid())
                    m.set_param(om.get_param_name(j), p);
                auto &pa = om.get_param_array(j);
                if (pa.is_valid())
                    m.set_param_array(om.get_param_name(j), pa);
            }
            if (!edge)
            {
                for (int j = 0; j < m.get_passes_count(); ++j)
                {
                    auto &p = m.get_pass(j);
                    p.get_state().set_cull_face(os.cull_face, os.cull_order);
                }

                auto shadow_param = nya_math::vec4(1.0f, 0.0f, 0.0f, 0.3f);

                m.set_param("light ambient", scene::instance().get_light_ambient());
                m.set_param("light color", scene::instance().get_light_color());
                m.set_param("light dir", scene::instance().get_light_dir());
                m.set_param_array("shadow tr", scene::instance().get_shadow_tr());
                m.set_param("shadow cascades", scene::instance().get_shadow_cascades());
                m.set_texture("shadow", scene::instance().get_shadow_tex());

                if (pmx)
                {
                    auto &pmx_mat = pmx->materials[i];
                    if (!pmx_mat.shadow_cast)
                    {
                        m.remove_pass("shadows");
                        if (pmx_mat.shadow_receive)
                            shadow_param.x = 0.0f;
                    }

                    if (!pmx_mat.shadow_receive)
                    {
                        m.set_texture("shadow", scene::instance().white_texture());
                        shadow_param = nya_math::vec4();
                    }
                }
                m.set_param("shadow param", shadow_param);
            }
            m.set_name(om.get_name());
            m_mesh.set_material(i, m);
        }
    }

    if (m_mesh.is_mmd())
    {
        m_mesh.set_scale(-mmd_scale(),mmd_scale(),-mmd_scale());
        enable_phys();
    }
    else
        m_mesh.set_scale(-1.0f,1.0f,-1.0f);

    nya_scene::material::set_resources_prefix(mat_prefix.c_str());

    return result;
}

int mesh::get_origin() const { return m_origin; }

void mesh::set_enabled(bool enabled) { m_enabled = enabled; }

void mesh::enable_phys()
{
    auto pos = m_mesh.get_pos();
    auto scale = m_mesh.get_scale();
    m_mesh.set_pos(pos / scale);
    m_mesh.set_scale(nya_math::vec3::one());
    m_phys.init(&m_mesh, scene::instance().mmd_phys());
    m_mesh.set_pos(pos);
    m_mesh.set_scale(scale);
}

void mesh::set_animation(nya_scene::animation_proxy anim, int layer)
{
    remove_blend(layer);
    m_mesh.set_anim(anim, layer, true);
    m_mesh.update(0);
    update_bones();
}

void mesh::blend_animation(nya_scene::animation_proxy anim, int layer, float duration)
{
    if (duration < 0.01f)
    {
        set_animation(anim, layer);
        return;
    }

    remove_blend(layer); //ToDo: multiple blends

    blend b;

    auto &prev = m_mesh.get_anim(layer);
    if (prev.is_valid())
    {
        b.prev_layer = new_blend_id(1024);
        b.prev_weight = prev->get_weight();
        m_mesh.set_anim(prev.get(), b.prev_layer, true);
        m_mesh.set_anim_time(m_mesh.get_anim_time(layer), b.prev_layer);
    }
    else //fade in
        b.prev_layer = -1;

    if (anim.is_valid())
    {
        b.layer = layer;
        b.weight = anim->get_weight();
        anim->set_weight(0.0f);
        m_mesh.set_anim(anim, layer, true);
    }
    else //fade out
        m_mesh.remove_anim(layer);

    if (!anim.is_valid() && b.prev_layer < 0)
        return;

    b.layer = layer;
    b.total_time = duration;
    b.time = 0;
    m_blends.push_back(b);

    m_mesh.update(0);
    update_bones();
}

int mesh::new_blend_id(int src)
{
    for (auto &b: m_blends)
    {
        if (b.prev_layer == src)
            return new_blend_id(src + 1);
    }
    return src;
}

void mesh::remove_blend(int layer)
{
    bool found = false;
    for (auto &b: m_blends)
    {
        if (b.layer != layer)
            continue;

        if (b.prev_layer >= 0)
            m_mesh.remove_anim(b.prev_layer);
        found = true;
    }

    if (!found)
        return;

    m_blends.erase(std::remove_if(m_blends.begin(), m_blends.end(), [layer](const blend &b) { return layer == b.layer; }), m_blends.end());
}

void mesh::remove_animation(int layer)
{
    remove_blend(layer);
    m_mesh.remove_anim(layer);
    m_mesh.update(0);
    update_bones();
}

int mesh::get_anim_time(int layer)
{
    return m_mesh.get_anim_time(layer);
}

void mesh::set_anim_time(int layer, int time)
{
    m_mesh.set_anim_time(time, layer);
    m_mesh.update(0);
    update_bones();
}

bool mesh::is_anim_finished(int layer)
{
    return m_mesh.is_anim_finished(layer);
}

bool mesh::bone::update(mmd_mesh &mesh)
{
    auto t = transform.lock();
    if (!t)
        return false;

    t->set_local_pos(mesh.get_bone_pos(bone_idx, true) * mesh.get_scale());
    auto r = mesh.get_bone_rot(bone_idx, true);
    r.v.x = -r.v.x;
    r.v.z = -r.v.z;
    t->set_local_rot(r);
    return true;
}

int mesh::get_bones_count() { return m_mesh.get_bones_count(); }
const char *mesh::get_bone_name(int idx) { return m_mesh.get_bone_name(idx); }

int mesh::get_bone(const char *name)
{
    m_bones.erase(std::remove_if(m_bones.begin(), m_bones.end(), bone::expired), m_bones.end());

    for (auto &b: m_bones)
    {
        if (b.name == name)
            return b.transform_idx;
    }

    bone b;
    b.name = name;
    b.bone_idx = m_mesh.get_bone_idx(name);
    b.transform_idx = transform::add();
    b.transform = transform::get_weak(b.transform_idx);
    b.transform.lock()->set_parent(m_torigin);
    b.update(m_mesh);

    m_bones.push_back(b);
    return b.transform_idx;
}

void mesh::set_bone_pos(const char *name, const nya_math::vec3 &pos, bool additive)
{
    m_mesh.set_bone_pos(m_mesh.get_bone_idx(name), pos, additive);
}

void mesh::set_bone_rot(const char *name, const nya_math::quat &rot, bool additive)
{
    auto r = rot;
    r.v.x = -r.v.x;
    r.v.z = -r.v.z;
    m_mesh.set_bone_rot(m_mesh.get_bone_idx(name), r, additive);
}

int mesh::get_morphs_count() { return m_mesh.get_morphs_count(); }
const char *mesh::get_morph_name(int idx) { return m_mesh.get_morph_name(idx); }

float mesh::get_morph(const char *name)
{
    const int idx = find_morph(name);
    if (idx < 0)
        return -9999;
    
    return m_mesh.get_morph(idx);
}

bool mesh::set_morph(const char *name, float v, bool override_animation)
{
    const int idx = find_morph(name);
    if (idx < 0)
        return false;

    m_mesh.set_morph(idx, v, override_animation);
    return true;
}

int mesh::get_groups_count()
{
    return m_groups_count;
}

const char *mesh::get_group_name(int idx)
{
    return m_mesh.get_material(idx).get_name();
}

void mesh::set_group_visible(int idx, bool value)
{
    if (m_groups_visible.empty())
        m_groups_visible.resize(m_mesh.get_groups_count(), true);
    m_groups_visible[idx] = value;

    auto & sh = m_mesh.internal().get_shared_data();
    if (!sh.is_valid())
        return;
    
    auto &g = sh->groups[idx];
    for (int i = 0, to = (int)sh->groups.size(); i < to; ++i)
    {
        auto &gg = sh->groups[i];
        if (g.offset == gg.offset && g.count == gg.count)
            m_groups_visible[i] = value;
    }
}

void mesh::mesh_init_texture(int idx, int tex_id)
{
    auto &m = m_mesh.modify_material(idx);
    auto p = m.get_texture("diffuse");
    auto &n = texture::get(tex_id)->tex;
    if (p.is_valid())
        n.set(p.get());
    m.set_texture("diffuse", n);
}

void mesh::mesh_set_texture(int idx, int tex_id)
{
    m_mesh.modify_material(idx).set_texture("diffuse", texture::get(tex_id)->tex);
}

void mesh::copy_to_shape(shape *s) const
{
    auto & sh = m_mesh.internal().get_shared_data();
    if (!sh.is_valid())
        return;

    auto &vbo = sh->vbo;
    const auto vcount = vbo.get_verts_count();
    if (!vcount)
        return;

    nya_memory::tmp_buffer_ref tmp;
    if (!vbo.get_vertex_data(tmp))
        return;

    std::vector<shape::vertex> verts(vcount);
    auto stride = vbo.get_vert_stride();
    auto pos_size = vbo.get_vert_dimension() * sizeof(float);
    auto pdata = (char *)tmp.get_data() + vbo.get_vert_offset();
    for (auto &v: verts)
    {
        memcpy(&v.pos.x, pdata, pos_size);
        pdata += stride;
    }
    if (vbo.get_normals_offset() > 0)
    {
        auto ndata = (char *)tmp.get_data() + vbo.get_normals_offset();
        for (auto &v: verts)
        {
            memcpy(&v.normal.x, ndata, sizeof(v.normal));
            ndata += stride;
        }
    }
    tmp.free();

    auto other_transform = transform::get(s->get_origin());
    nya_math::mat4 other_mat;
    other_mat.rotate(nya_math::quat::invert(other_transform->get_rot())).translate(-other_transform->get_pos())
    .translate(m_torigin->get_pos()).scale(m_mesh.get_scale()).rotate(m_torigin->get_rot());

    nya_math::mat4 other_rot_mat;
    other_mat.rotate(nya_math::quat::invert(other_transform->get_rot())).rotate(m_torigin->get_rot());

    for (auto &v: verts)
    {
        v.pos = other_mat * v.pos;
        v.normal = other_rot_mat * v.normal;
    }

    const auto icount = vbo.get_indices_count();
    if (icount > 0)
    {
        if (!vbo.get_index_data(tmp))
            return;

        if (m_groups_visible.empty())
        {
            if (vbo.get_index_size() == nya_render::vbo::index4b)
                s->add_tris(verts.data(), vcount, (uint32_t *)tmp.get_data(), icount);
            else
                s->add_tris(verts.data(), vcount, (uint16_t *)tmp.get_data(), icount);
        }
        else if (vbo.get_index_size() == nya_render::vbo::index4b)
        {
            const auto *src = (uint32_t *)tmp.get_data();
            std::vector<uint32_t> dst;
            for (int i = 0, to = (int)m_groups_visible.size(); i < to; ++i)
            {
                if (!m_groups_visible[i])
                    continue;
                auto &g = sh->groups[i];
                auto off = dst.size();
                dst.resize(off + g.count);
                memcpy(dst.data() + off, src + g.offset, g.count * sizeof(uint32_t));
            }
            s->add_tris(verts.data(), vcount, dst.data(), (uint32_t)dst.size());
        }
        else if (vbo.get_index_size() == nya_render::vbo::index2b)
        {
            const auto *src = (uint16_t *)tmp.get_data();
            std::vector<uint16_t> dst;
            for (int i = 0, to = (int)m_groups_visible.size(); i < to; ++i)
            {
                if (!m_groups_visible[i])
                    continue;
                auto &g = sh->groups[i];
                auto off = dst.size();
                dst.resize(off + g.count);
                memcpy(dst.data() + off, src + g.offset, g.count  * sizeof(uint16_t));
            }
            s->add_tris(verts.data(), vcount, dst.data(), (uint32_t)dst.size());
        }
    }
    else
        s->add_tris(verts.data(), vcount, (uint16_t *)0, icount);
}

void mesh::update_bones()
{
    bool bone_deleted = false;
    for (auto &b: m_bones)
    {
        if (!b.update(m_mesh))
            bone_deleted = true;
    }
    if (bone_deleted)
        m_bones.erase(std::remove_if(m_bones.begin(), m_bones.end(), bone::expired), m_bones.end());
}

void mesh::update_pre(int dt)
{
    if(!m_enabled)
        return;

    for (int i = (int)m_blends.size() - 1; i >= 0; --i)
    {
        auto &b = m_blends[i];
        b.time += dt * 0.001f;
        float k = b.time / b.total_time;
        if (k > 1.0f)
        {
            k = 1.0f;
            m_blends.erase(m_blends.begin() + i);
            
            if (b.prev_layer >= 0)
                m_mesh.remove_anim(b.prev_layer);
            
            auto anim = m_mesh.get_anim(b.layer);
            if (anim.is_valid())
                anim->set_weight(b.weight);
            continue;
        }

        if (b.prev_layer >= 0)
        {
            auto prev = m_mesh.get_anim(b.prev_layer);
            prev->set_weight(b.prev_weight * (1.0 - k));
        }

        auto anim = m_mesh.get_anim(b.layer);
        if (anim.is_valid())
            anim->set_weight(b.weight * k);
    }

    m_mesh.set_pos(m_torigin->get_pos());
    m_mesh.set_rot(m_torigin->get_rot());

    m_mesh.update(dt);
    update_bones();

    auto pos = m_mesh.get_pos();
    auto scale = m_mesh.get_scale();
    m_mesh.set_pos(pos / scale);
    m_mesh.set_scale(nya_math::vec3::one());
    if (m_first_update)
    {
        m_phys.reset();
        m_first_update = false;
    }
    m_phys.update_pre();
    m_mesh.set_pos(pos);
    m_mesh.set_scale(scale);
}

void mesh::update_post()
{
    if(!m_enabled)
        return;

    auto pos = m_mesh.get_pos();
    auto scale = m_mesh.get_scale();
    m_mesh.set_pos(pos / scale);
    m_mesh.set_scale(nya_math::vec3::one());
    m_phys.update_post();
    m_mesh.set_pos(pos);
    m_mesh.set_scale(scale);
    
    m_zorder = (nya_scene::get_camera().get_pos() - pos).length();
}

void mesh::draw(const char *pass)
{
    if (!m_enabled)
        return;

    if (m_groups_visible.empty())
    {
        m_mesh.draw(pass);
        return;
    }

    for (int i = 0; i < (int)m_groups_visible.size(); ++i)
    {
        if (m_groups_visible[i])
            m_mesh.draw_group(i, pass);
    }
}

mesh::mesh()
{
    scene::instance().reg_object(this);
    m_torigin = transform::get((m_origin = transform::add()));
}

mesh::~mesh()
{
    scene::instance().unreg_object(this);
    m_phys.release();
}
