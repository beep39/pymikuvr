//

#include "navigation.h"
#include "mesh.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourNavMeshBuilder.h"
#include "Recast.h"
#include <float.h>

typedef nya_math::vec3 vec3;

void navigation::set_debug(bool enable)
{
    m_shape.set_color(0.0f, 1.0f, 1.0f, enable ? 0.4f : 0.0f);
}

bool navigation::load(const char *name)
{
    auto data = nya_resources::read_data(name);

    m_navmesh = dtAllocNavMesh();
    if (!m_navmesh->init((unsigned char *)data.get_data(), (int)data.get_size(), true))
    {
        dtFreeNavMesh(m_navmesh);
        data.free();
        m_navmesh = 0;
        return false;
    }
    data.free();

    //ToDo: initialize m_shape

    return true;
}

bool navigation::trace(const nya_math::vec3 &origin, const nya_math::vec3 &dir, float &result) const
{
    if (!m_navquery)
        return false;

    return m_shape.trace(origin, dir, result);
}

bool navigation::nearest_point(const nya_math::vec3 &origin, float radius, nya_math::vec3 &result) const
{
    if (!m_navquery)
        return false;

    radius *= 2.0f;
    const float extends[] = {radius, radius, radius};

    dtPolyRef poly;
    if (m_navquery->findNearestPoly(&origin.x, extends, m_filter, &poly, 0) != DT_SUCCESS)
        return false;

    return m_navquery->closestPointOnPoly(poly, &origin.x, &result.x, 0) == DT_SUCCESS;
}

bool navigation::farthest_point(const vec3 &origin, const vec3 &dir_, float radius, vec3 &result) const
{
    if (!m_navquery)
        return false;
    
    dtPolyRef from_poly;
    const float extends[] = {0.1f, 1.0f, 0.1f};
    if (m_navquery->findNearestPoly(&origin.x, extends, m_filter, &from_poly, 0) != DT_SUCCESS)
        return false;
    
    vec3 start;
    if (m_navquery->closestPointOnPoly(from_poly, &origin.x, &start.x, 0) != DT_SUCCESS)
        return false;
    
    const int max_count = 128;
    dtPolyRef poly[max_count];
    //float cost[max_count];
    int result_count;
    if (m_navquery->findPolysAroundCircle(from_poly, &start.x, radius, m_filter, poly, 0, 0, &result_count, max_count) != DT_SUCCESS)
        return false;
    
    nya_math::vec3 dir = vec3::normalize(dir_);
    
    float result_dist = -1;

    vec3 end = start + dir * radius;
    for (int i = 0; i < result_count; ++i)
    {
        vec3 tmp_end;
        if (m_navquery->closestPointOnPoly(poly[i], &end.x, &tmp_end.x, 0) != DT_SUCCESS)
            continue;
        
        const float dist = (tmp_end - start).length();
        if (dist > result_dist && dist < radius + 0.01f)
        {
            result = tmp_end;
            result_dist = dist;
        }
    }

    return result_dist > 0;
}

int navigation::path(const nya_math::vec3 &from, const nya_math::vec3 &to, float *result, int result_array_size) const
{
    if (!m_navquery)
        return false;

    const float extends[] = {2, 2, 2};

    dtPolyRef from_poly;
    if (m_navquery->findNearestPoly(&from.x, extends, m_filter, &from_poly, 0) != DT_SUCCESS)
        return false;

    dtPolyRef to_poly;
    if (m_navquery->findNearestPoly(&to.x, extends, m_filter, &to_poly, 0) != DT_SUCCESS)
        return false;
    
    std::vector<dtPolyRef> path(result_array_size/3);

    int count = 0;
    if (m_navquery->findPath(from_poly, to_poly, &from.x, &to.x, m_filter, path.data(), &count, (int)path.size()) != DT_SUCCESS)
        return false;

    int vcount = 0;
    if (m_navquery->findStraightPath(&from.x, &to.x, path.data(), count, result, 0, 0, &vcount, result_array_size/3) != DT_SUCCESS)
        return false;

    return vcount;
}

void navigation::clear()
{
    m_shape.clear();
    release();
}

void navigation::add_mesh(int mesh_idx)
{
    mesh::get(mesh_idx)->copy_to_shape(&m_shape);
}

void navigation::add_shape(int shape_idx)
{
    m_shape.add_shape(shape::get(shape_idx));
}

bool navigation::build(const params &params_)
{
    release();

    vec3 rc_bmin(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 rc_bmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    uint32_t rc_nverts = (uint32_t)m_shape.verts().size();
    std::vector<vec3> rc_verts(rc_nverts);
    uint32_t rc_ntris = (uint32_t)(m_shape.inds2b().size() + m_shape.inds4b().size()) / 3;
    std::vector<int> rc_tris(rc_ntris * 3);
    std::vector<vec3> rc_trinorms(rc_ntris);

    auto &src_verts = m_shape.verts();
    for (uint32_t i = 0; i < rc_verts.size(); ++i)
    {
        auto &v = rc_verts[i] = src_verts[i].pos;
        rc_bmax = vec3::max(v, rc_bmax);
        rc_bmin = vec3::min(v, rc_bmin);
    }
    if (!m_shape.inds2b().empty())
    {
        auto &src = m_shape.inds2b();
        for (uint32_t i = 0, to = rc_ntris * 3; i < to; i += 3)
        {
            rc_tris[i] = src[i];
            rc_tris[i + 1] = src[i + 2];
            rc_tris[i + 2] = src[i + 1];
        }
    }
    if (!m_shape.inds4b().empty())
    {
        auto &src = m_shape.inds4b();
        for (uint32_t i = 0, to = rc_ntris * 3; i < to; i += 3)
        {
            rc_tris[i] = src[i];
            rc_tris[i + 1] = src[i + 2];
            rc_tris[i + 2] = src[i + 1];
        }
    }
    for (uint32_t n = 0, i = 0; n < rc_ntris; ++n)
    {
        std::swap(rc_tris[i], rc_tris[i + 1]);
        
        auto &v0 = rc_verts[rc_tris[i++]];
        auto &v1 = rc_verts[rc_tris[i++]];
        auto &v2 = rc_verts[rc_tris[i++]];
        rc_trinorms[n] = vec3::cross(v0 - v2, v1 - v2).normalize();
    }

    auto ctx = rcContext(true) ;

    // Step 1. Initialize build config.

    const float cs = params_.cell_size;
    const float ch = params_.cell_height;
    const float walkable_slope_angle = params_.max_slope;
    const int walkable_height = (int)ceilf(params_.agent_height / ch);
    const int walkable_climb = (int)floorf(params_.max_climb / ch);
    const int walkable_radius = (int)ceilf(params_.agent_radius / cs);
    const int max_edge_len = (int)(params_.max_edge_length / cs);
    const float max_simplification_error = params_.max_edge_error;
    const int min_region_area = (int)rcSqr(params_.min_region_size);
    const int merge_region_area = (int)rcSqr(params_.merged_region_size);
    const int maxVertsPerPoly = DT_VERTS_PER_POLYGON;
    const float detail_sample_dist = cs * 6.0f;
    const float detail_sample_max_error = ch * 1.0f;

    // Reset build times gathering.
    ctx.resetTimers();

    // Start the build process.
    ctx.startTimer(RC_TIMER_TOTAL);

    // Set the area where the navigation will be build.
    // Here the bounds of the input mesh are used, but the
    // area could be specified by an user defined box, etc.
    int width, height;
    rcCalcGridSize(&rc_bmin.x, &rc_bmax.x, cs, &width, &height);

    // Step 2. Rasterize input polygon soup.

    // Allocate voxel heightfield where we rasterize our input data to.
    auto solid = rcAllocHeightfield();
    if (!rcCreateHeightfield(&ctx, *solid, width, height, &rc_bmin.x, &rc_bmax.x, cs, ch))
    {
        rcFreeHeightField(solid);
        return false;
    }

    // Array that can hold triangle area types.
    std::vector<unsigned char> triareas(rc_ntris, 0);

    // Find triangles which are walkable based on their slope and rasterize them.
    // If your input data is multiple meshes, you can transform them here, calculate
    // the are type for each of the meshes and rasterize them.
    rcMarkWalkableTriangles(&ctx, walkable_slope_angle, &rc_verts.data()->x, rc_nverts, rc_tris.data(), rc_ntris, triareas.data());
    rcRasterizeTriangles(&ctx, &rc_verts.data()->x, rc_nverts, rc_tris.data(), triareas.data(), rc_ntris, *solid, walkable_climb);
    triareas.clear();

    // Step 3. Filter walkables surfaces.
    
    // Once all geoemtry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    rcFilterLowHangingWalkableObstacles(&ctx, walkable_climb, *solid);
    rcFilterLedgeSpans(&ctx, walkable_height, walkable_climb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, walkable_height, *solid);

    // Step 4. Partition walkable surface to simple regions.
    
    // Compact the heightfield so that it is faster to handle from now on.
    // This will result more cache coherent data as well as the neighbours
    // between walkable cells will be calculated.
    auto chf = rcAllocCompactHeightfield();
    if (!rcBuildCompactHeightfield(&ctx, walkable_height, walkable_climb, *solid, *chf))
    {
        rcFreeCompactHeightfield(chf);
        rcFreeHeightField(solid);
        return false;
    }

    rcFreeHeightField(solid);

    // Erode the walkable area by agent radius.
    if (!rcErodeWalkableArea(&ctx, walkable_radius, *chf))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // Prepare for region partitioning, by calculating distance field along the walkable surface.
    if (!rcBuildDistanceField(&ctx, *chf))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }
    
    // Partition the walkable surface into simple regions without holes.
    if (!rcBuildRegions(&ctx, *chf, 0, min_region_area, merge_region_area))
    {
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // Step 5. Trace and simplify region contours.
    
    auto contour_set = rcAllocContourSet();
    if (!rcBuildContours(&ctx, *chf, max_simplification_error, max_edge_len, *contour_set))
    {
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(contour_set);
        return false;
    }

    // Step 6. Build polygons mesh from contours.

    auto pmesh = rcAllocPolyMesh();
    if (!rcBuildPolyMesh(&ctx, *contour_set, maxVertsPerPoly, *pmesh))
    {
        rcFreePolyMesh(pmesh);
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(contour_set);
        return false;
    }

    // Step 7. Create detail mesh which allows to access approximate height on each polygon.

    auto dmesh = rcAllocPolyMeshDetail();
    if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, detail_sample_dist, detail_sample_max_error, *dmesh))
    {
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(contour_set);
        return false;
    }

    rcFreeCompactHeightfield(chf);
    rcFreeContourSet(contour_set);

    // At this point the navigation mesh data is ready, you can access it from pmesh.
    // See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

    // (Optional) Step 8. Create Detour data from Recast poly mesh.

    unsigned char* nav_data = 0;
    int nav_data_size = 0;

    // Update poly flags from areas.
    for (int i = 0; i < pmesh->npolys; ++i)
    {
        if (pmesh->areas[i] == RC_WALKABLE_AREA)
        {
            pmesh->areas[i] = 0; //POLYAREA_GROUND
            pmesh->flags[i] = 0x01; //POLYFLAGS_WALK
        }
    }

    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = pmesh->verts;
    params.vertCount = pmesh->nverts;
    params.polys = pmesh->polys;
    params.polyAreas = pmesh->areas;
    params.polyFlags = pmesh->flags;
    params.polyCount = pmesh->npolys;
    params.nvp = pmesh->nvp;
    params.detailMeshes = dmesh->meshes;
    params.detailVerts = dmesh->verts;
    params.detailVertsCount = dmesh->nverts;
    params.detailTris = dmesh->tris;
    params.detailTriCount = dmesh->ntris;

    const int MAX_OFFMESH_CONNECTIONS = 256;
    float off_mesh_con_verts[MAX_OFFMESH_CONNECTIONS*3*2];
    float off_mesh_con_rads[MAX_OFFMESH_CONNECTIONS];
    unsigned char off_mesh_con_dirs[MAX_OFFMESH_CONNECTIONS];
    unsigned char off_mesh_con_areas[MAX_OFFMESH_CONNECTIONS];
    unsigned short off_mesh_con_flags[MAX_OFFMESH_CONNECTIONS];
    unsigned int off_mesh_con_id[MAX_OFFMESH_CONNECTIONS];
    int off_mesh_con_count = 0;

    // no off mesh connections yet
    params.offMeshConVerts = off_mesh_con_verts ;
    params.offMeshConRad = off_mesh_con_rads ;
    params.offMeshConDir = off_mesh_con_dirs ;
    params.offMeshConAreas = off_mesh_con_areas ;
    params.offMeshConFlags = off_mesh_con_flags ;
    params.offMeshConUserID = off_mesh_con_id ;
    params.offMeshConCount = off_mesh_con_count ;

    params.walkableHeight = params_.agent_height;
    params.walkableRadius = params_.agent_radius;
    params.walkableClimb = params_.max_climb;
    rcVcopy(params.bmin, pmesh->bmin);
    rcVcopy(params.bmax, pmesh->bmax);
    params.cs = cs;
    params.ch = ch;

    if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size))
    {
        rcFreePolyMeshDetail(dmesh);
        rcFreePolyMesh(pmesh);
        return false;
    }

    m_navmesh = dtAllocNavMesh();
    if (!m_navmesh)
    {
        rcFreePolyMeshDetail(dmesh);
        rcFreePolyMesh(pmesh);
        dtFree(nav_data);
        return false;
    }

    dtStatus status;
    status = m_navmesh->init(nav_data, nav_data_size, DT_TILE_FREE_DATA);
    if (dtStatusFailed(status))
    {
        dtFreeNavMesh(m_navmesh);
        m_navmesh = 0;
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        dtFree(nav_data);
        return false;
    }

    m_navquery = dtAllocNavMeshQuery();
    status = m_navquery->init(m_navmesh, 2048);
    if (dtStatusFailed(status))
    {
        release();
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    ctx.stopTimer(RC_TIMER_TOTAL);

    create_debug_mesh(*pmesh);
    rcFreePolyMesh(pmesh);
    rcFreePolyMeshDetail(dmesh);
    return true ;
}

navigation::navigation()
{
    set_debug(false);
    m_filter = new dtQueryFilter();
    m_filter->setIncludeFlags(0xFFFF);
    m_filter->setExcludeFlags(0);
    m_filter->setAreaCost(0, 1.0f);
}

navigation::~navigation()
{
    clear();
    delete m_filter;
}

void navigation::release()
{
    if (m_navmesh)
    {
        dtFreeNavMesh(m_navmesh);
        m_navmesh = 0;
    }

    if (m_navquery)
    {
        dtFreeNavMeshQuery(m_navquery);
        m_navquery = 0;
    }
}

void navigation::create_debug_mesh(const struct rcPolyMesh &mesh)
{
    const int nvp = mesh.nvp;
    const float cs = mesh.cs;
    const float ch = mesh.ch;
    const float* orig = mesh.bmin;

    std::vector<shape::vertex> verts;
    std::vector<unsigned short> inds;

    int idx = 0;
    if(mesh.npolys)
    {
        for (int i = 0; i < mesh.npolys; ++i)
        {
            if (mesh.areas[i] == 0)
            {
                const unsigned short* p = &mesh.polys[i*nvp*2];
                unsigned short vi[3];
                for (int j = 2; j < nvp; ++j)
                {
                    if (p[j] == RC_MESH_NULL_IDX)
                        break;
                    vi[0] = p[0];
                    vi[1] = p[j];
                    vi[2] = p[j-1];
                    for (int k = 0; k < 3; ++k)
                    {
                        const unsigned short* v = &mesh.verts[vi[k]*3];
                        shape::vertex vert;
                        vert.pos.x = orig[0] + v[0]*cs;
                        vert.pos.y = orig[1] + (v[1])*ch;
                        vert.pos.z = orig[2] + v[2]*cs;
                        verts.push_back(vert);
                    }
                    inds.push_back(idx++);
                    inds.push_back(idx++);
                    inds.push_back(idx++);
                }
            }
        }

        //ToDo: boundary lines
/*
        for (int i = 0; i < mesh.npolys; ++i)
        {
            const unsigned short* p = &mesh.polys[i*nvp*2];
            for (int j = 0; j < nvp; ++j)
            {
                if (p[j] == RC_MESH_NULL_IDX) break;
                if (p[nvp+j] != RC_MESH_NULL_IDX) continue;
                int vi[2];
                vi[0] = p[j];
                if (j+1 >= nvp || p[j+1] == RC_MESH_NULL_IDX)
                    vi[1] = p[0];
                else
                    vi[1] = p[j+1];
                for (int k = 0; k < 2; ++k)
                {
                    const unsigned short* v = &mesh.verts[vi[k]*3];
                    vec3 pos;
                    pos.x = orig[0] + v[0]*cs;
                    pos.y = orig[1] + (v[1]+1)*ch;
                    pos.z = orig[2] + v[2]*cs;
                }
            }
        }
*/
    }

    m_shape.clear();
    m_shape.add_tris(verts.data(), (uint32_t)verts.size(), inds.data(), (uint32_t)inds.size());
}
