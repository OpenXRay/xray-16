#include "LevelVisibility.h"
#include "xrCDB/Frustum.h"

#include <cmath>
#include <algorithm>

namespace xray::render::vulkan
{
bool select_visible_sector_roots(const std::vector<LevelSector>& sectors,
    const std::vector<LevelPortal>& portals, size_t visual_count, size_t camera_sector,
    const Fmatrix& view_projection, const Fvector& camera_position,
    std::vector<uint32_t>& roots)
{
    roots.clear();
    if (sectors.empty() || camera_sector >= sectors.size() || portals.empty() ||
        !std::isfinite(camera_position.x) || !std::isfinite(camera_position.y) ||
        !std::isfinite(camera_position.z))
        return false;

    const float* matrix_values = reinterpret_cast<const float*>(&view_projection);
    for (size_t index = 0; index < 16; ++index)
        if (!std::isfinite(matrix_values[index]))
            return false;

    Fmatrix full_transform = view_projection;
    CFrustum camera_frustum;
    camera_frustum.CreateFromMatrix(full_transform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
    if (!camera_frustum.p_count || camera_frustum.p_count > FRUSTUM_MAXPLANES)
        return false;
    for (size_t plane = 0; plane < camera_frustum.p_count; ++plane)
    {
        const auto& p = camera_frustum.planes[plane];
        if (!std::isfinite(p.n.x) || !std::isfinite(p.n.y) ||
            !std::isfinite(p.n.z) || !std::isfinite(p.d))
            return false;
    }

    std::vector<uint32_t> pending{static_cast<uint32_t>(camera_sector)};
    std::vector<uint32_t> selected;
    std::vector<uint8_t> visited_sectors(sectors.size(), 0);
    std::vector<uint8_t> processed_portals(portals.size(), 0);
    std::vector<uint8_t> added_roots(visual_count, 0);
    visited_sectors[camera_sector] = 1;

    while (!pending.empty())
    {
        const uint32_t current = pending.back();
        pending.pop_back();
        const LevelSector& sector = sectors[current];
        if (sector.root >= visual_count)
            return false;
        if (!added_roots[sector.root])
        {
            selected.push_back(sector.root);
            added_roots[sector.root] = 1;
        }

        for (uint16_t portal_id : sector.portals)
        {
            if (portal_id >= portals.size())
                return false;
            if (processed_portals[portal_id])
                continue;

            const LevelPortal& portal = portals[portal_id];
            if (portal.vertices.size() < 3 || portal.vertices.size() > 6 ||
                portal.sector_front >= sectors.size() || portal.sector_back >= sectors.size() ||
                (portal.sector_front != current && portal.sector_back != current) ||
                !std::isfinite(portal.center[0]) || !std::isfinite(portal.center[1]) ||
                !std::isfinite(portal.center[2]) || !std::isfinite(portal.radius) || portal.radius < 0.f)
                return false;

            Fvector sphere_center;
            sphere_center.set(portal.center[0], portal.center[1], portal.center[2]);
            if (!camera_frustum.testSphere_dirty(sphere_center, portal.radius))
                continue;

            sPoly source, clipped;
            for (const auto& point : portal.vertices)
            {
                Fvector vertex;
                vertex.set(point[0], point[1], point[2]);
                source.push_back(vertex);
            }
            sPoly* visible_polygon = camera_frustum.ClipPoly(source, clipped);
            if (!visible_polygon || visible_polygon->size() < 3)
                continue;

            const uint32_t destination = portal.sector_front == current ?
                portal.sector_back : portal.sector_front;
            processed_portals[portal_id] = 1;
            if (!visited_sectors[destination])
            {
                visited_sectors[destination] = 1;
                pending.push_back(destination);
            }
        }
    }
    roots = std::move(selected);
    return !roots.empty();
}

void append_unsectored_level_roots(const std::vector<uint32_t>& level_roots,
    const std::vector<LevelSector>& sectors, size_t visual_count,
    std::vector<uint32_t>& visible_roots)
{
    std::vector<uint8_t> sector_owned(visual_count, 0), added(visual_count, 0);
    for (const LevelSector& sector : sectors)
        if (sector.root < visual_count) sector_owned[sector.root] = 1;
    for (uint32_t root : visible_roots)
        if (root < visual_count) added[root] = 1;
    for (uint32_t root : level_roots)
        if (root < visual_count && !sector_owned[root] && !added[root])
        {
            visible_roots.push_back(root);
            added[root] = 1;
        }
}
}
