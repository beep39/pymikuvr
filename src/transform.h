//

#pragma once

#include "math/vector.h"
#include "math/quaternion.h"
#include "container.h"

class transform: public container<transform>
{
public:
    unsigned int version() { return m_version; }

    const nya_math::vec3 &get_pos();
    const nya_math::quat &get_rot();

    void set_pos(const nya_math::vec3 &p);
    void set_rot(const nya_math::quat &r);

    const nya_math::vec3 &get_local_pos();
    const nya_math::quat &get_local_rot();

    void set_local_pos(const nya_math::vec3 &p);
    void set_local_rot(const nya_math::quat &r);

    bool set_parent(transform *parent, bool relative = true);

    ~transform();

private:
    void set_dirty();

    void set_rel_pos(const nya_math::vec3 &p);
    void set_rel_rot(const nya_math::quat &r);

    bool find_parent(transform *t);

private:
    unsigned int m_version = 0;
    
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;

    nya_math::vec3 m_pos_offset;
    nya_math::quat m_rot_offset;

    bool m_dirty = false;

    transform *m_parent = 0;
    std::vector<transform*> m_children;
};
