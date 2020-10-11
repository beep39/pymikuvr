//

#include "animation.h"
#include "sound.h"

void animation::set_mesh(int mesh_idx, int layer)
{
    m_mesh = mesh::get_weak(mesh_idx);
    m_layer = layer;
    auto m = mesh::get(mesh_idx);
    m->set_animation(anim, layer);
    m->set_anim_time(m_layer, m_time);
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

void animation::copy(int from, int to)
{
    get(to)->anim.set(get(from)->anim.get());
    get(to)->set_time(get(from)->m_time);
}
