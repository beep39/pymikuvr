//

#include "animation.h"

void animation::set_mesh(int mesh_idx, int layer)
{
    m_mesh = mesh::get_weak(mesh_idx);
    m_layer = layer;
    auto m = mesh::get(mesh_idx);
    m->set_animation(anim, layer);
    m->set_anim_time(m_layer, m_time);
}

int animation::load(const char *name)
{
    const int result = anim->load(name) ? anim->get_duration() : -1;
    auto m = m_mesh.lock();
    if (m)
    {
        m->set_animation(anim, m_layer);
        m->set_anim_time(m_layer, 0);
    }
    return result;
}

int animation::get_time()
{
    auto m = m_mesh.lock();
    return m ? m->get_anim_time(m_layer) : m_time;
}

void animation::set_time(int time)
{
    auto m = m_mesh.lock();
    if (m)
        m->set_anim_time(m_layer, time);
    else
        m_time = time;
}

int animation::set_range(int from, int to)
{
    anim->set_range(from, to);
    auto duration = (int)anim->get_duration();
    if(from>duration)
        from=duration;
    if(to>duration)
        to=duration;
    if(from>to)
        from=to;
    return to-from;
}

bool animation::is_finished()
{
    auto m = m_mesh.lock();
    if (m)
        return m->is_anim_finished(m_layer);
    return true;
}

inline bool replace(std::string& str, const std::string& from, const std::string& to)
{
    const size_t s = str.find(from);
    if (s == std::string::npos)
        return false;
    str.replace(s, from.length(), to);
    return true;
}

void animation::mirror()
{
    if (!anim.is_valid())
        return;
    
    const auto &sh = anim->get_shared_data();
    if (!sh.is_valid())
        return;

    nya_scene::shared_animation sha;
    const auto &from = sh->anim;
    auto &to = sha.anim;

    const std::string left = "\xe5\xb7\xa6", right = "\xe5\x8f\xb3";

    for (int i = 0, count = from.get_bones_count(); i < count; ++i)
    {
        std::string name = from.get_bone_name(i);
        if (!replace(name, left, right))
            replace(name, right, left);
        to.add_bone(name.c_str());
    }

    for (int i = 0, count = from.get_bones_count(); i < count; ++i)
    {
        for (auto &f: from.get_pos_frames(i))
        {
            nya_math::vec3 pos = f.second.value;
            pos.x = -pos.x;
            to.add_bone_pos_frame(i, f.first, pos, f.second.inter);
        }

        for (auto &f: from.get_rot_frames(i))
        {
            nya_math::quat rot = f.second.value;
            rot.v.y = -rot.v.y;
            rot.v.z = -rot.v.z;
            to.add_bone_rot_frame(i, f.first, rot, f.second.inter);
        }
    }

    for (int i = 0, count = from.get_cuves_count(); i < count; ++i)
    {
        std::string name = from.get_curve_name(i);
        if(!replace(name, left, right))
            replace(name, right, left);
        to.add_curve(name.c_str());
    }

    for (int i = 0, count = from.get_cuves_count(); i < count; ++i)
    {
        for (auto &f: from.get_curve_frames(i))
            to.add_curve_frame(i, f.first, f.second.value);
    }

    anim->create(sha);
}

void animation::copy(int from, int to)
{
    get(to)->anim.set(get(from)->anim.get());
    get(to)->set_time(get(from)->m_time);
}
