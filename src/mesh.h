//

#pragma once

#include "tests/shared/mmd_phys.h"
#include "tests/shared/mmd_mesh.h"
#include "scene.h"
#include "transform.h"

class shape;

class mesh: public container<mesh>, private scene::iobject
{
public:
    bool load(const char *name);
    int get_origin() const;
    void set_enabled(bool enabled);

    int get_anim_time(int layer);
    void set_anim_time(int layer, int time);
    void set_animation(nya_scene::animation_proxy anim, int layer);
    void blend_animation(nya_scene::animation_proxy anim, int layer, float duration);
    void blend_animation(nya_scene::animation_proxy anim, int layer, float duration, nya_scene::animation_proxy transition, int time);
    void remove_animation(int layer);
    bool is_anim_finished(int layer);

    int get_bones_count();
    const char *get_bone_name(int idx);
    int get_bone(const char *name);
    void set_bone_pos(const char *name, const nya_math::vec3 &pos, bool additive);
    void set_bone_rot(const char *name, const nya_math::quat &rot, bool additive);
    void bone_pos_transformed(int transform_id);
    void bone_rot_transformed(int transform_id);
    void reset_bone(int transform_id);

    int get_morphs_count();
    const char *get_morph_name(int idx);
    float get_morph(const char *name);
    bool set_morph(const char *name, float v, bool override_animation);

    int get_groups_count();
    const char *get_group_name(int idx);
    void set_group_visible(int idx, bool visible);
    bool material_load(int idx, const char *name);
    void init_texture(int idx, int tex_id);
    void set_texture(int idx, int tex_id);

    void enable_phys(bool enable);
    void enable_reset();

    void copy_to_shape(shape *s) const;

public:
    mesh();
    ~mesh();

private:
    void draw(const char *pass);
    void update_pre(int dt);
    void update_post();
    float zorder() const { return m_zorder; }
    const nya_math::aabb &get_aabb() const { return m_mesh.get_aabb(); }

private:
    void update_bones();

    void enable_phys(mmd_phys_world &w)
    {
        auto pos = m_mesh.get_pos();
        auto scale = m_mesh.get_scale();
        m_mesh.set_pos(pos / scale);
        m_mesh.set_scale(nya_math::vec3::one());
        m_phys.init(&m_mesh, w.get_world());
        m_mesh.set_pos(pos);
        m_mesh.set_scale(scale);
    }

    int find_morph(std::string name)
    {
        std::transform(name.begin(),name.end(),name.begin(),::tolower);
        for (int i = 0; i < m_mesh.get_morphs_count(); ++i)
        {
            if (strcicmp(m_mesh.get_morph_name(i), name.c_str()) == 0)
            return i;
        }
        return -1;
    }

    static int strcicmp(char const *a, char const *b)
    {
        for (;; a++, b++) {
            int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
            if (d != 0 || !*a)
            return d;
        }
    }
    
    int new_blend_id(int src);
    void remove_blend(int layer);

private:
    mmd_mesh m_mesh;
    mmd_phys_controller m_phys;
    int m_origin;
    transform *m_torigin;
    bool m_enabled = true;
    float m_zorder = 0;
    std::vector<bool> m_groups_visible;
    int m_groups_count = 0;
    bool m_first_update = false;

    struct bone
    {
        std::string name;
        int parent_idx;
        int bone_idx;
        int transform_id;
        std::weak_ptr<transform> transform;
        nya_math::vec3 offset;

        bool to_transform(mmd_mesh &mesh);
        void pos_from_transform(mmd_mesh &mesh);
        void rot_from_transform(mmd_mesh &mesh);

        static bool expired(const bone &b) { return b.transform.expired(); }
    };
    std::vector<bone> m_bones;

    struct blend
    {
        int layer = -1;
        int prev_layer = -1;
        float time = 0;
        float total_time = 0;
        float prev_weight = 1.0f;
        float weight = 1.0f;
        bool anim = false;
        int set_time = 0;
    };
    std::vector<blend> m_blends;
};
