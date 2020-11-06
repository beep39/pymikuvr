//

#pragma once

#include "scene/material.h"
#include "container.h"

class texture: public container<texture>
{
public:
    nya_scene::texture_proxy tex;
    texture() { tex.create(); }

    static void init();

    bool save(const char *name);
    void build(int w, int h, float r, float g, float b, float a);
    
    static void copy(int from, int to);
};
