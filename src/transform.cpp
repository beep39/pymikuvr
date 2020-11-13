//

#include "transform.h"
#include <list>

namespace { const float precision = 0.000001f; }

const nya_math::vec3 &transform::get_pos()
{
    if (m_dirty)
    {
        m_dirty = false;
        m_pos = m_parent->get_pos() + m_parent->get_rot().rotate(m_pos_offset);
        m_rot = m_parent->get_rot() * m_rot_offset;
    }
    return m_pos;
}

const nya_math::quat &transform::get_rot()
{
    if (m_dirty)
    {
        m_dirty = false;
        m_pos = m_parent->get_pos() + m_parent->get_rot().rotate(m_pos_offset);
        m_rot = m_parent->get_rot() * m_rot_offset;
    }
    return m_rot;
}

void transform::set_pos(const nya_math::vec3 &p)
{
    if (m_parent)
        get_pos(); //update

    if (fabsf(p.x - m_pos.x) +
        fabsf(p.y - m_pos.y) +
        fabsf(p.z - m_pos.z) < precision)
        return;

    m_pos = p;
    if (m_parent)
        set_rel_pos(p);

    for (auto &c: m_children)
        c->set_dirty();

    ++m_version;
}

void transform::set_rot(const nya_math::quat &r)
{
    if (m_parent)
        get_rot(); //update

    if (fabsf(r.v.x - m_rot.v.x) +
        fabsf(r.v.y - m_rot.v.y) +
        fabsf(r.v.z - m_rot.v.z) +
        fabsf(r.w - m_rot.w) < precision)
        return;

    m_rot = r;
    if (m_parent)
        set_rel_rot(r);

    for (auto &c: m_children)
        c->set_dirty();

    ++m_version;
}

const nya_math::vec3 &transform::get_local_pos()
{
    return m_parent ? m_pos_offset : m_pos;
}

const nya_math::quat &transform::get_local_rot()
{
    return m_parent ? m_rot_offset : m_rot;
}

void transform::set_local_pos(const nya_math::vec3 &p)
{
    if (fabsf(p.x - m_pos_offset.x) +
        fabsf(p.y - m_pos_offset.y) +
        fabsf(p.z - m_pos_offset.z) < precision)
        return;

    if (m_parent)
    {
        m_pos_offset = p;
        set_dirty();
    }
    else
    {
        m_pos = p;
        for (auto &c: m_children)
            c->set_dirty();
        ++m_version;
    }
}

void transform::set_local_rot(const nya_math::quat &r)
{
    if (fabsf(r.v.x - m_rot_offset.v.x) +
        fabsf(r.v.y - m_rot_offset.v.y) +
        fabsf(r.v.z - m_rot_offset.v.z) +
        fabsf(r.w - m_rot_offset.w) < precision)
        return;

    if (m_parent)
    {
        m_rot_offset = r;
        set_dirty();
    }
    else
    {
        m_rot = r;
        for (auto &c: m_children)
            c->set_dirty();
        ++m_version;
    }
}

bool transform::set_parent(transform *parent, bool relative)
{
    if (m_parent)
    {
        for (int i = 0; i < (int)m_parent->m_children.size(); ++i)
        {
            if (m_parent->m_children[i] == this)
            {
                m_parent->m_children.erase(m_parent->m_children.begin() + i);
                break;
            }
        }

        get_pos();
        get_rot();
        m_pos_offset = nya_math::vec3();
        m_rot_offset = nya_math::quat();
        m_parent = 0;
    }

    if (parent)
    {
        parent->m_children.push_back(this);
        m_parent = parent;
        if (relative)
        {
            set_rel_pos(m_pos);
            set_rel_rot(m_rot);
        }
        else
        {
            m_pos_offset = m_pos;
            m_rot_offset = m_rot;
            for (auto &c: m_children)
                c->set_dirty();
        }
        ++m_version;
    }

    return true;
}

void transform::set_dirty()
{
    if (m_dirty)
        return;

    m_dirty = true;
    ++m_version;
    for (auto &c: m_children)
        c->set_dirty();
}

void transform::set_rel_pos(const nya_math::vec3 &p)
{
    m_pos_offset = m_parent->get_rot().rotate_inv(p - m_parent->get_pos());
}

void transform::set_rel_rot(const nya_math::quat &r)
{
    m_rot_offset = nya_math::quat::invert(m_parent->get_rot()) * r;
}

transform::~transform()
{
    if (m_parent)
        set_parent(0, false);
    for (auto &c: m_children)
        c->set_parent(0);
}
