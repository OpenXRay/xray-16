#include "xrEngine/stdafx.h"
#include "GpuLevel.h"
#include "VulkanVisual.h"

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
    GpuLevel prepared;
    prepared.device_ = device;
    prepared.pool_ = pool;
    prepared.upload_ = upload;
    prepared.visuals_ = std::move(models.visuals);
    prepared.roots_ = std::move(models.roots);
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

void GpuLevel::destroy()
{
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    visual_objects_.clear();
    meshes_.clear();
    visuals_.clear();
    roots_.clear();
    pending_.clear();
    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    upload_ = {};
}
}
