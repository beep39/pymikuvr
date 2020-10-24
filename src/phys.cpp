//

#include "phys.h"
#include "mesh.h"
#include "shape.h"

btDefaultCollisionConfiguration *phys::m_col_conf;
btCollisionDispatcher *phys::m_dispatcher;
btBroadphaseInterface *phys::m_broadphase;
btSequentialImpulseConstraintSolver *phys::m_solver;
btDiscreteDynamicsWorld *phys::m_world;

btRigidBody *phys::m_ground = 0;
btMotionState *phys::m_ground_state = 0;
btCollisionShape *phys::m_ground_shape = 0;

bullet_debug_draw phys::m_phys_draw;
bool phys::m_debug_enabled = false;

std::vector<phys *> phys::m_update_list;

typedef nya_math::vec3 vec3;

inline btVector3 convert(const vec3& from) { return btVector3(from.x,from.y,from.z); }
inline btQuaternion convert(const nya_math::quat& from) { return btQuaternion(from.v.x,from.v.y,from.v.z,from.w); }
inline vec3 convert(const btVector3 &from) { return vec3(from.x(),from.y(),from.z()); }
inline nya_math::quat convert(const btQuaternion &from) { return nya_math::quat(from.x(),from.y(),from.z(),from.w()); }

void phys::init()
{
    m_col_conf = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_col_conf);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();
    m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_col_conf);
    m_world->setGravity(btVector3(0, -9.8f, 0));

    set_ground(0.0f, true);
}

void phys::update(int dt)
{
    if (m_update_list.empty())
        return;

    for (auto &p: m_update_list)
    {
        if (p->m_tversion == p->m_torigin->version())
            continue;

        btTransform tr;
        tr.setOrigin(convert(p->m_torigin->get_pos()));
        tr.setRotation(convert(p->m_torigin->get_rot()));
        
        if (p->m_kinetic)
            p->m_motion_state->setWorldTransform(tr);
        else
            p->m_rigid_body->setCenterOfMassTransform(tr);
        p->m_tversion = p->m_torigin->version();
    }

    m_world->stepSimulation(dt * 0.001f, 10);

    for (auto &p: m_update_list)
    {
        if (p->m_kinetic || !p->m_enabled)
            continue;
    
        const btTransform &tr = p->m_rigid_body->getCenterOfMassTransform();
        p->m_torigin->set_pos(convert(tr.getOrigin()));
        p->m_torigin->set_rot(convert(tr.getRotation()));
        p->m_tversion = p->m_torigin->version();
    }
}

void phys::release()
{
    set_ground(0, false);
    delete m_world;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_col_conf;
}

void phys::debug_draw()
{
    if (m_debug_enabled)
        m_world->debugDrawWorld();
}

void phys::set_debug(bool enable)
{
    if (m_debug_enabled == enable)
        return;

    if (enable)
    {
        m_world->setDebugDrawer(&m_phys_draw);
        m_phys_draw.set_debug_mode(btIDebugDraw::DBG_DrawWireframe);
        m_phys_draw.set_alpha(0.7f);
    }
    else
        m_world->setDebugDrawer(0);
    m_debug_enabled = enable;
}

void phys::set_ground(float y, bool enable)
{
    if (!enable)
    {
        if (!m_ground)
            return;

        m_world->removeRigidBody(m_ground);
        delete m_ground_state;
        delete m_ground;
        delete m_ground_shape;
        m_ground=0;
        return;
    }

    if (m_ground)
        set_ground(y, false);

    m_ground_state = new btDefaultMotionState();
    m_ground_shape = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), y);
    btRigidBody::btRigidBodyConstructionInfo bt_info(0.0f, m_ground_state, m_ground_shape, btVector3(0, 0, 0));
    bt_info.m_linearDamping = 0.0f;
    bt_info.m_angularDamping = 0.0f;
    bt_info.m_restitution = 0.0f;
    bt_info.m_friction = 0.265f;
    m_ground = new btRigidBody(bt_info);

    m_ground->setCollisionFlags(m_ground->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_ground->setActivationState(DISABLE_DEACTIVATION);
    m_world->addRigidBody(m_ground);
}

void phys::set_box(float x, float y, float z, float m)
{
    set(new btBoxShape(btVector3(x * 0.5f, y * 0.5f, z * 0.5f)), m);
}

void phys::set_sphere(float r, float m)
{
    set(new btSphereShape(r), m);
}

void phys::set_capsule(float r, float h, float m)
{
    set(new btCapsuleShape(r, h), m);
}

void phys::set_cylinder(float r, float h, float m)
{
    set(new btCylinderShape(btVector3(r * 0.5f, h * 0.5f, r * 0.5f)), m);
}

void phys::set_cone(float r, float h, float m)
{
    set(new btConeShape(r, h), m);
}

void phys::set_tris(vec3 *verts, int vcount, int *indices, int icount, float m)
{
    m_verts.resize(vcount);
    memcpy(m_verts.data(), verts, vcount*sizeof(vec3));
    m_indices.resize(icount);
    memcpy(m_indices.data(), indices, icount*sizeof(int));

    const auto vdata = (btScalar *)&m_verts.data()->x;
    const int nverts = (int)m_verts.size();
    const int vstride = (int)sizeof(vec3);
    const int ntris = icount / 3;
    const int tstride = (int)sizeof(int) * 3;

    m_tri_array = new btTriangleIndexVertexArray(ntris, m_indices.data(), tstride, nverts, vdata, vstride);
    set(new btBvhTriangleMeshShape(m_tri_array, true, true), 0.0f, true);
}

void phys::set(btCollisionShape *shape, float m, bool static_)
{
    m_kinetic = m < 0.00001f;

    m_col_shape = shape;
    btVector3 local_inertia(0,0,0);
    if (!m_kinetic)
        m_col_shape->calculateLocalInertia(m, local_inertia);

    btTransform transform;
    transform.setIdentity();
    m_motion_state = new btDefaultMotionState(transform);

    btRigidBody::btRigidBodyConstructionInfo rb_info(m, m_motion_state, m_col_shape, local_inertia);
    m_rigid_body = new btRigidBody(rb_info);

    if (static_)
    {
        m_rigid_body->setCollisionFlags(m_rigid_body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    }
    else if (m_kinetic)
    {
        m_rigid_body->setCollisionFlags(m_rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_rigid_body->setActivationState(DISABLE_DEACTIVATION);
    }

    if (m_enabled)
        m_world->addRigidBody(m_rigid_body);
}

void phys::clear()
{
    if (m_build_shape)
    {
        delete m_build_shape;
        m_build_shape = 0;
    }
}

void phys::add_mesh(int mesh_idx)
{
    if (!m_build_shape)
    {
        m_build_shape = new shape();
        m_build_shape->set_enabled(false);
    }

    mesh::get(mesh_idx)->copy_to_shape(m_build_shape);
}

void phys::add_shape(int shape_idx)
{
    if (!m_build_shape)
    {
        m_build_shape = new shape();
        m_build_shape->set_enabled(false);
    }

    m_build_shape->add_shape(shape::get(shape_idx));
}

bool phys::build(float m)
{
    if (!m_build_shape)
        return false;

    uint32_t nverts = (uint32_t)m_build_shape->verts().size();
    if (nverts > 0)
    {
        m_verts.resize(nverts);
        auto *src_vert = m_build_shape->verts().data();
        for (auto &v: m_verts)
            v = src_vert++->pos;

        const int vstride = (int)sizeof(vec3);
        const auto vdata = (btScalar *)&m_verts.data()->x;
        const int ntris = (int)(m_build_shape->inds2b().size() + m_build_shape->inds4b().size()) / 3;
        const int tstride = (int)sizeof(int) * 3;

        m_indices.resize(ntris * 3);
        if (!m_build_shape->inds2b().empty())
        {
            auto *ind = m_build_shape->inds2b().data();
            for (auto &t: m_indices)
                t = *ind++;
        }
        else
        {
            auto *ind = m_build_shape->inds4b().data();
            for (auto &t: m_indices)
                t = *ind++;
        }
        m_tri_array = new btTriangleIndexVertexArray(ntris, m_indices.data(), tstride, nverts, vdata, vstride);
        set(new btBvhTriangleMeshShape(m_tri_array, true, true), 0.0f, true);
    }
    delete m_build_shape;
    m_build_shape = 0;
    return true;
}

bool phys::trace(const nya_math::vec3 &origin, const nya_math::vec3 &dir, float *result, int *id, float max_dist)
{
    const btVector3 from = convert(origin);
    const btVector3 to = convert(origin + dir * max_dist);
    btCollisionWorld::ClosestRayResultCallback ray_result(from, to);
    m_world->rayTest(from, to, ray_result);
    if (!ray_result.hasHit())
        return false;

    if (result)
        *result = (ray_result.m_hitPointWorld - from).length();

    if (id)
    {
        *id = -1;
        for (auto &o: m_update_list)
        {
            if (o->m_col_shape == ray_result.m_collisionObject->getCollisionShape())
            {
                *id = get_idx(o);
                break;
            }
        }
    }
    return true;
}

bool phys::trace(const nya_math::vec3 &origin, const nya_math::vec3 &dir, float *result, float max_dist) const
{
    if (!m_rigid_body)
        return false;

    const btVector3 from = convert(origin);
    const btVector3 to = convert(origin + dir * max_dist);
    btTransform tfrom;
    tfrom.setIdentity();
    tfrom.setOrigin(from);
    btTransform tto;
    tto.setIdentity();
    tto.setOrigin(to);
    btCollisionWorld::ClosestRayResultCallback ray_result(from, to);
    btCollisionWorld::rayTestSingle(tfrom, tto, m_rigid_body, m_col_shape, m_rigid_body->getWorldTransform(), ray_result);
    if (!ray_result.hasHit())
        return false;

    *result = (ray_result.m_hitPointWorld - from).length();
    return true;
}

int phys::get_origin() const { return m_origin; }

void phys::set_enabled(bool enabled)
{
    if (m_enabled == enabled || !m_rigid_body)
        return;

    if (enabled)
        m_world->addRigidBody(m_rigid_body);
    else
        m_world->removeRigidBody(m_rigid_body);
    m_enabled = enabled;
}

phys::phys()
{
    m_torigin = transform::get((m_origin = transform::add()));
    m_update_list.push_back(this);
    m_tversion = m_torigin->version();
}

phys::~phys()
{
    m_update_list.erase(std::remove(m_update_list.begin(), m_update_list.end(), this), m_update_list.end());

    if (m_build_shape)
    {
        delete m_build_shape;
        m_build_shape = 0;
    }

    if (!m_rigid_body)
        return;

    if (m_enabled)
        m_world->removeRigidBody(m_rigid_body);

    delete m_motion_state;
    delete m_rigid_body;
    delete m_col_shape;

    if (m_tri_array)
        delete m_tri_array;
}
