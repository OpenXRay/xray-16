#include "xrEngine/stdafx.h"
#include "GpuLevel.h"
#include "LevelVisibility.h"
#include "VulkanVisual.h"
#include "SpecialVisuals.h"
#include "xrEngine/device.h"

#include <algorithm>
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
    prepared.textures_ = &textures;
    prepared.pass_ = &pass;
    prepared.visuals_ = std::move(models.visuals);
    prepared.roots_ = std::move(models.roots);
    prepared.sectors_ = std::move(models.sectors);
    prepared.portals_ = std::move(models.portals);
    prepared.meshes_.reserve(models.models.size());
    const auto upload_mesh = [&](const LevelModel& model, Mesh& mesh) -> bool
    {
        if (model.material >= models.materials.size() || model.vertices.empty() ||
            model.indices.empty() || model.indices.size() > std::numeric_limits<uint32_t>::max())
        {
            error = "invalid or empty Vulkan level model";
            return false;
        }
        if (!textures.material(models.materials[model.material].textures, pass, mesh.material, error) ||
            !upload_buffer(device, queue, pool, memory, upload,
                model.vertices.data(), model.vertices.size() * sizeof(LevelVertex),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertices, prepared.pending_, error) ||
            !upload_buffer(device, queue, pool, memory, upload,
                model.indices.data(), model.indices.size() * sizeof(uint32_t),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indices, prepared.pending_, error))
            return false;
        mesh.index_count = static_cast<uint32_t>(model.indices.size());
        mesh.windows = model.windows;
        mesh.mode = models.materials[model.material].mode;
        return true;
    };
    for (const LevelModel& model : models.models)
    {
        prepared.meshes_.emplace_back();
        Mesh& mesh = prepared.meshes_.back();
        if (!upload_mesh(model, mesh))
            return false;
        if (model.fast)
        {
            mesh.fast = std::make_unique<Mesh>();
            if (!upload_mesh(*model.fast, *mesh.fast))
                return false;
        }
    }
    // The old level may still have been submitted. Its owner must wait for
    // those frames before replacing the level. Uploads within prepared use
    // the same queue as the first draw and preserve staging until retired.
    destroy();
    device_ = prepared.device_;
    pool_ = prepared.pool_;
    upload_ = prepared.upload_;
    textures_ = prepared.textures_;
    pass_ = prepared.pass_;
    pending_ = std::move(prepared.pending_);
    meshes_ = std::move(prepared.meshes_);
    visuals_ = std::move(prepared.visuals_);
    roots_ = std::move(prepared.roots_);
    sectors_ = std::move(prepared.sectors_);
    portals_ = std::move(prepared.portals_);
    visual_objects_.reserve(visuals_.size());
    for (size_t index = 0; index < visuals_.size(); ++index)
    {
        visual_objects_.emplace_back(std::make_unique<VulkanVisual>(*this,
            static_cast<uint32_t>(index), visuals_[index]));
        visual_indices_.emplace(visual_objects_.back().get(), static_cast<uint32_t>(index));
    }
    prepared.device_ = VK_NULL_HANDLE;
    prepared.textures_ = nullptr;
    prepared.pass_ = nullptr;
    prepared.pool_ = VK_NULL_HANDLE;
    error.clear();
    return true;
}

bool GpuLevel::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16]) const
{
    for (GeometryPhase phase : {GeometryPhase::OpaqueAndAlphaTest, GeometryPhase::Transparent})
        if (!record(frame, pass, mvp, phase)) return false;
    return true;
}

bool GpuLevel::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16], GeometryPhase phase) const
{
    for (uint32_t root : roots_)
        if (!record_visual(root, frame, pass, mvp, phase)) return false;
    return true;
}

bool GpuLevel::record_visual(size_t index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16], GeometryPhase phase, float lod) const
{
    if (index >= visuals_.size()) return false;
    const LevelVisual& visual = visuals_[index];
    if (visual.type == 6 && lod < .33f)
    {
        Fvector direction;
        direction.set(visual.bounds[6] - Device.vCameraPosition.x,
            visual.bounds[7] - Device.vCameraPosition.y,
            visual.bounds[8] - Device.vCameraPosition.z);
        const auto facet = select_lod_facet(visual.lod_normals, {direction.x, direction.y, direction.z});
        const int32_t mesh_index = visual.lod_facets[facet];
        if (mesh_index < 0 || size_t(mesh_index) >= meshes_.size()) return false;
        const Mesh& mesh = meshes_[mesh_index];
        const bool transparent = mesh.mode == SurfaceMode::Transparent;
        const char* vertex = transparent ? "vk\\object_blended.vs" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.vs" : "vk\\level_opaque.vs";
        const char* fragment = transparent ? "vk\\object_blended.ps" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.ps" : "vk\\level_opaque.ps";
        const bool named = pass.has_game_pipeline(vertex, fragment);
        if (phase == GeometryPhase::Transparent ? transparent : !transparent)
            return transparent ? pass.record_transparent(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, mvp, mesh.material, 0, named ? vertex : nullptr, named ? fragment : nullptr) :
                pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    mesh.index_count, mvp, mesh.material, mesh.mode, 0, named ? vertex : nullptr, named ? fragment : nullptr);
        return true;
    }
    if (visual.mesh >= 0)
    {
        if (static_cast<size_t>(visual.mesh) >= meshes_.size()) return false;
        const Mesh& base = meshes_[visual.mesh];
        const Mesh& mesh = base.fast && lod < 0.33f ? *base.fast : base;
        const bool transparent = mesh.mode == SurfaceMode::Transparent;
        const bool selected = phase == GeometryPhase::Transparent ? transparent :
            phase == GeometryPhase::OpaqueAndAlphaTest && !transparent;
        const SlideWindow window = select_slide_window(mesh.windows, lod, mesh.index_count);
        const char* vertex = transparent ? "vk\\object_blended.vs" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.vs" :
            (&mesh != &base) ? "vk\\progressive_opaque.vs" :
            (visual.type == 7 || visual.type == 11) ? "vk\\tree_opaque.vs" : "vk\\level_opaque.vs";
        const char* fragment = transparent ? "vk\\object_blended.ps" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.ps" : "vk\\level_opaque.ps";
        const bool named = pass.has_game_pipeline(vertex, fragment);
        if (selected && !(transparent ?
                pass.record_transparent(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, window.offset,
                    named ? vertex : nullptr, named ? fragment : nullptr) :
                pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, mesh.mode, window.offset,
                    named ? vertex : nullptr, named ? fragment : nullptr))) return false;
    }
    for (uint32_t child : visual.children)
        if (!record_visual(child, frame, pass, mvp, phase, lod)) return false;
    return true;
}

bool GpuLevel::record_hud_visual(size_t index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16], float lod) const
{
    if (index >= visuals_.size()) return false;
    const LevelVisual& visual = visuals_[index];
    if (visual.type == 6 && lod < .33f)
    {
        Fvector direction;
        direction.set(visual.bounds[6] - Device.vCameraPosition.x,
            visual.bounds[7] - Device.vCameraPosition.y,
            visual.bounds[8] - Device.vCameraPosition.z);
        const auto facet = select_lod_facet(visual.lod_normals, {direction.x, direction.y, direction.z});
        const int32_t mesh_index = visual.lod_facets[facet];
        if (mesh_index < 0 || size_t(mesh_index) >= meshes_.size()) return false;
        const Mesh& mesh = meshes_[mesh_index];
        return pass.record_hud(frame, mesh.vertices.handle(), mesh.indices.handle(),
            mesh.index_count, mvp, mesh.material);
    }
    if (visual.mesh >= 0)
    {
        if (static_cast<size_t>(visual.mesh) >= meshes_.size()) return false;
        const Mesh& base = meshes_[visual.mesh];
        const Mesh& mesh = base.fast && lod < 0.33f ? *base.fast : base;
        const SlideWindow window = select_slide_window(mesh.windows, lod, mesh.index_count);
        if (!pass.record_hud(frame, mesh.vertices.handle(), mesh.indices.handle(),
                window.index_count, mvp, mesh.material, window.offset)) return false;
    }
    for (uint32_t child : visual.children)
        if (!record_hud_visual(child, frame, pass, mvp, lod)) return false;
    return true;
}

IRenderVisual* GpuLevel::get_visual(size_t index) const
{
    return index < visual_objects_.size() ? visual_objects_[index].get() : nullptr;
}

int GpuLevel::find_visual_index(const IRenderVisual* visual) const
{
    const auto it = visual_indices_.find(visual);
    return it == visual_indices_.end() ? -1 : static_cast<int>(it->second);
}

const LevelVisual* GpuLevel::visual_node(size_t index) const
{
    return index < visuals_.size() ? &visuals_[index] : nullptr;
}

bool GpuLevel::visible_sector_roots(size_t camera_sector, const Fmatrix& view_projection,
    const Fvector& camera_position, std::vector<uint32_t>& roots) const
{
    return select_visible_sector_roots(sectors_, portals_, visuals_.size(),
        camera_sector, view_projection, camera_position, roots);
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
    ++revision_;
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    visual_indices_.clear();
    visual_objects_.clear();
    if (textures_ && pass_)
        for (const Mesh& mesh : meshes_)
        {
            textures_->release_material(mesh.material, *pass_);
            if (mesh.fast)
                textures_->release_material(mesh.fast->material, *pass_);
        }
    meshes_.clear();
    visuals_.clear();
    roots_.clear();
    sectors_.clear();
    portals_.clear();
    pending_.clear();
    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    upload_ = {};
    textures_ = nullptr;
    pass_ = nullptr;
}
}
