#pragma once

#include "BufferUpload.h"
#include "DeferredPass.h"
#include "EngineLevelModels.h"
#include "GameTextureFactory.h"
#include "xrCore/_matrix.h"
#include <array>
#include <memory>
#include <unordered_map>

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
        GeometryPhase phase = GeometryPhase::OpaqueAndAlphaTest, float lod = 1.f,
        bool cull_static = true) const;
    bool record_hud_visual(size_t index, const FrameRecordingContext& frame,
        const DeferredPass& pass, const float (&mvp)[16], float lod = 1.f) const;
    size_t model_count() const { return meshes_.size(); }
    uint64_t geometry_bytes() const
    {
        const auto mesh_bytes = [](const Mesh& mesh)
        {
            uint64_t bytes = mesh.vertices.size() + mesh.indices.size();
            if (mesh.fast) bytes += mesh.fast->vertices.size() + mesh.fast->indices.size();
            return bytes;
        };
        uint64_t bytes = 0;
        for (const auto& mesh : meshes_) bytes += mesh_bytes(mesh);
        for (const auto& mesh : detail_meshes_) bytes += mesh_bytes(mesh);
        return bytes;
    }
    size_t visual_count() const { return visuals_.size(); }
    bool has_water() const { return has_water_; }
    uint64_t revision() const { return revision_; }
    IRenderVisual* get_visual(size_t index) const;
    int find_visual_index(const IRenderVisual* visual) const;
    const LevelVisual* visual_node(size_t index) const;
    bool visible_sector_roots(size_t camera_sector, const Fmatrix& view_projection,
        const Fvector& camera_position, std::vector<uint32_t>& roots) const;
    void all_level_roots(std::vector<uint32_t>& roots) const;
    void prepare_details(const Fvector& camera_position);
    bool record_details(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16]) const;
    bool record_sun_shadow(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&sun_mvp)[16]) const;
    void destroy();

private:
    struct Mesh
    {
        BufferResource vertices, indices;
        uint32_t index_count{};
        std::vector<SlideWindow> windows;
        VkDescriptorSet material{};
        SurfaceMode mode{SurfaceMode::Opaque};
        int alpha_ref{128};
        bool wallmark_multiply{};
        bool glow_alpha_add{};
        bool fog{};
        bool lightmapped{};
        bool water{};
        bool glass{};
        std::unique_ptr<Mesh> fast;
    };
    struct DetailBatch
    {
        BufferResource vertices, indices;
        uint32_t index_count{};
    };
    VkDevice device_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    VkCommandPool pool_{};
    BufferUploadDispatch upload_{};
    GameTextureFactory* textures_{};
    DeferredPass* pass_{};
    std::vector<PendingBufferUpload> pending_;
    std::vector<Mesh> meshes_;
    struct DetailPlacement
    {
        uint8_t model{};
        float x{}, y{}, z{}, yaw{}, scale{};
    };
    DetailAssets details_;
    std::vector<Mesh> detail_meshes_;
    std::unordered_map<size_t, std::vector<DetailPlacement>> detail_cache_;
    std::vector<DetailPlacement> visible_details_;
    // Each frame slot is reused only after FrameContext has waited on its fence.
    mutable std::array<std::vector<DetailBatch>, FrameContext::FramesInFlight> detail_batches_;
    mutable std::array<uint64_t, FrameContext::FramesInFlight> detail_signatures_{};
    std::vector<LevelVisual> visuals_;
    std::vector<uint8_t> visual_phases_; // opaque/cutout bit 1, transparent bit 2
    std::vector<uint32_t> roots_;
    std::vector<LevelSector> sectors_;
    std::vector<LevelPortal> portals_;
    std::vector<std::unique_ptr<VulkanVisual>> visual_objects_;
    std::unordered_map<const IRenderVisual*, uint32_t> visual_indices_;
    uint64_t revision_{};
    bool has_water_{};
};
}
