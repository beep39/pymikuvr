//

#pragma once

#include "container.h"
#include "shape.h"

class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;
struct rcPolyMesh;

class navigation: public container<navigation>
{
    typedef nya_math::vec3 vec3;
    
public:
    void set_debug(bool enable);

    bool load(const char *name);
    bool trace(const vec3 &origin, const vec3 &dir, float &result) const;
    bool nearest_point(const vec3 &origin, float radius, vec3 &result) const;
    bool farthest_point(const vec3 &origin, const vec3 &dir, float radius, vec3 &result) const;
    int path(const vec3 &from, const vec3 &to, float *result, int result_array_size) const;

    struct params
    {
        float cell_size = 0.3f;
        float cell_height = 0.2f;

        float agent_height = 2.0f;
        float agent_radius = 0.6f;
        float max_climb = 0.9f;
        float max_slope = 45.0f;

        float min_region_size = 8.0f;
        float merged_region_size = 20.0f;

        float max_edge_length = 12.0f;
        float max_edge_error = 1.3f;
    };

    void clear();
    void add_mesh(int mesh);
    void add_shape(int shape);
    bool build(const params &params);

    navigation();
    ~navigation();

private:
    void create_debug_mesh(const struct rcPolyMesh& mesh);
    void release();

private:
    dtNavMesh *m_navmesh = 0;
    dtNavMeshQuery *m_navquery = 0;
    dtQueryFilter *m_filter = 0;
    shape m_shape;
    bool m_building = false;
};
