//

#pragma once

#include "extensions/bullet.h"
#include "transform.h"

class shape;

class phys: public container<phys>
{
public:
    static void init();
    static void update(int dt);
    static void release();

    static void debug_draw();
    static void set_debug(bool enable);
    static void set_ground(float y, bool enable);

    void set_box(float x, float y, float z, float m);
    void set_sphere(float r, float m);
    void set_capsule(float r, float h, float m);
    void set_cylinder(float r, float h, float m);
    void set_cone(float r, float h, float m);
    void set_tris(nya_math::vec3 *verts, int vcount, int *indices, int icount, float m);

    void clear();
    void add_mesh(int src);
    void add_shape(int src);
    bool build(float m);

    static bool trace(const nya_math::vec3 &origin, const nya_math::vec3 &dir, float *result, int *id, float max_dist);
    bool trace(const nya_math::vec3 &origin, const nya_math::vec3 &dir, float *result, float max_dist) const;

    int get_origin() const;
    void set_enabled(bool enabled);

    phys();
    ~phys();

private:
    void set(btCollisionShape *shape, float m, bool static_ = false);

    int m_origin;
    transform *m_torigin;
    unsigned int m_tversion;
    bool m_enabled = true;

    shape *m_build_shape = 0;

    btMotionState *m_motion_state = 0;
    btCollisionShape *m_col_shape = 0;
    btRigidBody *m_rigid_body = 0;
    btTriangleIndexVertexArray *m_tri_array = 0;
    std::vector<nya_math::vec3> m_verts;
    std::vector<int> m_indices;
    bool m_kinetic = true;

    static std::vector<phys *> m_update_list;

private:
    static bool m_debug_enabled;

    static btDefaultCollisionConfiguration *m_col_conf;
    static btCollisionDispatcher *m_dispatcher;
    static btBroadphaseInterface *m_broadphase;
    static btSequentialImpulseConstraintSolver *m_solver;
    static btDiscreteDynamicsWorld *m_world;
    static bullet_debug_draw m_phys_draw;

    static btRigidBody *m_ground;
    static btMotionState *m_ground_state;
    static btCollisionShape *m_ground_shape;
};
