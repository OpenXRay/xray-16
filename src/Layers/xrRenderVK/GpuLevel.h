#pragma once

#include "BufferUpload.h"
#include "DeferredPass.h"
#include "EngineLevelModels.h"
#include "GameTextureFactory.h"
#include "xrCore/_matrix.h"
#include <memory>

class IRenderVisual;

namespace xray::render::vulkan
{
class VulkanVisual;
// GPU ownership of the supported static level subset. A failed load leaves
// the previous level intact; no partially uploaded scene is exposed.
class GpuLevel
{
public:
    GpuLevel() = default;
    ~GpuLevel();
    GpuLevel(const GpuLevel&) = delete;
    GpuLevel& operator=(const GpuLevel&) = delete;

    bool load(IReader& level, VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
        GameTextureFactory& textures, DeferredPass& pass, std::string& error);
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16]) const;
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16], GeometryPhase phase) const;
    bool record_visual(size_t index, const FrameRecordingContext& frame,
        const DeferredPass& pass, const float (&mvp)[16],
        GeometryPhase phase = GeometryPhase::OpaqueAndAlphaTest) const;
    bool record_hud_visual(size_t index, const FrameRecordingContext& frame,
        const DeferredPass& pass, const float (&mvp)[16]) const;
    size_t model_count() const { return meshes_.size(); }
    size_t visual_count() const { return visuals_.size(); }
    IRenderVisual* get_visual(size_t index) const;
    const LevelVisual* visual_node(size_t index) const;
    bool visible_sector_roots(size_t camera_sector, const Fmatrix& view_projection,
        const Fvector& camera_position, std::vector<uint32_t>& roots) const;
    void all_level_roots(std::vector<uint32_t>& roots) const;
    void destroy();

private:
    struct Mesh
    {
        BufferResource vertices, indices;
        uint32_t index_count{};
        VkDescriptorSet material{};
        SurfaceMode mode{SurfaceMode::Opaque};
    };
    VkDevice device_{};
    VkCommandPool pool_{};
    BufferUploadDispatch upload_{};
    std::vector<PendingBufferUpload> pending_;
    std::vector<Mesh> meshes_;
    std::vector<LevelVisual> visuals_;
    std::vector<uint32_t> roots_;
    std::vector<LevelSector> sectors_;
    std::vector<LevelPortal> portals_;
    std::vector<std::unique_ptr<VulkanVisual>> visual_objects_;
};
}
