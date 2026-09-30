#include "xrEngine/stdafx.h"
#include "GpuLevel.h"
#include "VulkanVisual.h"
#include "xrCDB/Frustum.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xray::render::vulkan
{
GpuLevel::~GpuLevel() { destroy(); }

bool GpuLevel::load(IReader& level, VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
    GameTextureFactory& textures, DeferredPass& pass, std::string& error)
{
    if (!device || !queue || !pool)
    {
        error = "Vulkan level requires a device and command pool";
        return false;
    }
    LevelModelData models;
    if (!load_engine_level_models(level, models, error)) return false;
    // Portal data is an optimization input. If an old or incomplete level
    // omits it, the renderer uses the complete root list rather than hiding
    // geometry based on partial visibility information.
    std::string visibility_error;
    if (!load_engine_level_visibility(level, models, visibility_error))
        Msg("! Vulkan: level visibility data unavailable (%s); rendering all level roots",
            visibility_error.c_str());
    GpuLevel prepared;
    prepared.device_ = device;
    prepared.pool_ = pool;
    prepared.upload_ = upload;
    prepared.visuals_ = std::move(models.visuals);
    prepared.roots_ = std::move(models.roots);
    prepared.sectors_ = std::move(models.sectors);
    prepared.portals_ = std::move(models.portals);
    prepared.meshes_.reserve(models.models.size());
    for (const LevelModel& model : models.models)
    {
        if (model.material >= models.materials.size() || model.vertices.empty() ||
            model.indices.empty() || model.indices.size() > std::numeric_limits<uint32_t>::max())
        {
            error = "invalid or empty Vulkan level model";
            return false;
        }
        prepared.meshes_.emplace_back();
        Mesh& mesh = prepared.meshes_.back();
        if (!textures.material(models.materials[model.material].textures, pass, mesh.material, error) ||
            !upload_buffer(device, queue, pool, memory, upload,
                model.vertices.data(), model.vertices.size() * sizeof(LevelVertex),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertices, prepared.pending_, error) ||
            !upload_buffer(device, queue, pool, memory, upload,
                model.indices.data(), model.indices.size() * sizeof(uint32_t),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indices, prepared.pending_, error))
            return false;
        mesh.index_count = static_cast<uint32_t>(model.indices.size());
    }
    // The old level may still have been submitted. Its owner must wait for
    // those frames before replacing the level. Uploads within prepared use
    // the same queue as the first draw and preserve staging until retired.
    destroy();
    device_ = prepared.device_;
    pool_ = prepared.pool_;
    upload_ = prepared.upload_;
    pending_ = std::move(prepared.pending_);
    meshes_ = std::move(prepared.meshes_);
    visuals_ = std::move(prepared.visuals_);
    roots_ = std::move(prepared.roots_);
    sectors_ = std::move(prepared.sectors_);
    portals_ = std::move(prepared.portals_);
    visual_objects_.reserve(visuals_.size());
    for (size_t index = 0; index < visuals_.size(); ++index)
        visual_objects_.emplace_back(std::make_unique<VulkanVisual>(*this,
            static_cast<uint32_t>(index), visuals_[index]));
    prepared.device_ = VK_NULL_HANDLE;
    prepared.pool_ = VK_NULL_HANDLE;
    error.clear();
    return true;
}

bool GpuLevel::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16]) const
{
    for (uint32_t root : roots_)
        if (!record_visual(root, frame, pass, mvp)) return false;
    return true;
}

bool GpuLevel::record_visual(size_t index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16]) const
{
    if (index >= visuals_.size()) return false;
    const LevelVisual& visual = visuals_[index];
    if (visual.mesh >= 0)
    {
        if (static_cast<size_t>(visual.mesh) >= meshes_.size()) return false;
        const Mesh& mesh = meshes_[visual.mesh];
        if (!pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, mvp, mesh.material)) return false;
    }
    for (uint32_t child : visual.children)
        if (!record_visual(child, frame, pass, mvp)) return false;
    return true;
}

IRenderVisual* GpuLevel::get_visual(size_t index) const
{
    return index < visual_objects_.size() ? visual_objects_[index].get() : nullptr;
}

const LevelVisual* GpuLevel::visual_node(size_t index) const
{
    return index < visuals_.size() ? &visuals_[index] : nullptr;
}

bool GpuLevel::visible_sector_roots(size_t camera_sector, const Fmatrix& view_projection,
    const Fvector& camera_position, std::vector<uint32_t>& roots) const
{
    roots.clear();
    if (sectors_.empty() || camera_sector >= sectors_.size() || portals_.empty() ||
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
    std::vector<uint8_t> visited_sectors(sectors_.size(), 0);
    std::vector<uint8_t> processed_portals(portals_.size(), 0);
    std::vector<uint8_t> added_roots(visuals_.size(), 0);
    visited_sectors[camera_sector] = 1;

    while (!pending.empty())
    {
        const uint32_t current = pending.back();
        pending.pop_back();
        const LevelSector& sector = sectors_[current];
        if (sector.root >= visuals_.size())
            return false;
        if (!added_roots[sector.root])
        {
            roots.push_back(sector.root);
            added_roots[sector.root] = 1;
        }

        for (uint16_t portal_id : sector.portals)
        {
            if (portal_id >= portals_.size())
                return false;
            if (processed_portals[portal_id])
                continue;

            const LevelPortal& portal = portals_[portal_id];
            if (portal.vertices.size() < 3 || portal.vertices.size() > 6 ||
                portal.sector_front >= sectors_.size() || portal.sector_back >= sectors_.size() ||
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
            if (!visible_polygon)
                continue;
            if (visible_polygon->size() < 3)
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
    return !roots.empty();
}

void GpuLevel::all_level_roots(std::vector<uint32_t>& roots) const
{
    roots.clear();
    std::vector<uint8_t> added(visuals_.size(), 0);
    const auto add = [&](uint32_t root)
    {
        if (root < visuals_.size() && !added[root])
        {
            roots.push_back(root);
            added[root] = 1;
        }
    };
    for (uint32_t root : roots_)
        add(root);
    for (const LevelSector& sector : sectors_)
        add(sector.root);
}

void GpuLevel::destroy()
{
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    visual_objects_.clear();
    meshes_.clear();
    visuals_.clear();
    roots_.clear();
    sectors_.clear();
    portals_.clear();
    pending_.clear();
    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    upload_ = {};
}
}
