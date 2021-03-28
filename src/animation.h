//

#pragma once

#include "mesh.h"

class animation: public container<animation>
{
public:
    nya_scene::animation_proxy anim;
    animation() { anim.create(); anim->set_loop(false); }

    void set_mesh(int mesh_idx, int layer);
    int load(const char *name);
    void blend(animation &new_anim, float duration);
    void blend(animation &new_anim, float duration, animation &transition);
    int get_time();
    void set_time(int time);
    int set_range(int from, int to);
    bool is_finished();
    void mirror();
    static void copy(int from, int to);

private:
    std::weak_ptr<mesh> m_mesh;
    int m_layer = -1;
    int m_time = 0;
};
