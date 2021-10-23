//

#pragma once

#include "scene/material.h"
#include "container.h"

class material: public container<material>
{
public:
    nya_scene::material mat;

    static void copy(int from, int to);

    bool load(const char *name);
    void set_texture(const char *type, int tex);
    void set_param(const char *type, float x, float y, float z, float w);
    void set_opaque(float alpha);

    //inline bool opaque() const { return m_opaque; }
    //inline bool alpha_clip() const { return m_alpha_clip; }
    //inline bool visible() const { return m_enabled && !m_zero_alpha; }

    bool set(const char *pass);
    void unset();

private:
    void update_opaque();

    bool m_enabled = true;
    bool m_opaque = true;
    bool m_zero_alpha = false;
    bool m_alpha_clip = false;
    float m_alpha = 1.0f;
};
