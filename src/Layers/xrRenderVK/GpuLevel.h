#pragma once

#include "BufferUpload.h"
#include "DeferredPass.h"
#include "EngineLevelModels.h"
#include "GameTextureFactory.h"
#include "LevelPassGraph.h"
#include "ResourceIssueCounter.h"
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
        bool cull_static = true, bool secondary_only = false) const;
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
    bool has_secondary_priority(size_t index) const
    { return index < visual_phases_.size() && (visual_phases_[index] & 4u) != 0; }
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
    bool prepare_transparent_details(const FrameRecordingContext& frame,
        const float (&mvp)[16], std::vector<float>& sort_distances) const;
    bool record_transparent_detail(size_t draw_index, const FrameRecordingContext& frame,
        const DeferredPass& pass, const float (&mvp)[16]) const;
    bool record_sun_shadow(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&sun_mvp)[16]) const;
    void destroy();

private:
    void report_visual_record_failure(size_t visual, int32_t mesh, GeometryPhase phase,
        bool water, uint32_t indices, float lod, const char* reason) const;
    struct Mesh
    {
        BufferResource vertices, indices;
        uint32_t index_count{};
        std::vector<SlideWindow> windows;
        VkDescriptorSet material{};
        uint32_t material_id{};
        std::string shader;
        SurfaceMode mode{SurfaceMode::Opaque};
        bool secondary_opaque{};
        int alpha_ref{128};
        bool wallmark_multiply{};
        bool glow_alpha_add{};
        bool fog{};
        bool lightmapped{};
        bool terrain{};
        bool surface{};
        bool single_detail{};
        bool environment{};
        bool laemb{};
        bool environment_constant{};
        LevelPassGraph pass_graph;
        std::array<WaveForm, 4> constant_waveforms{};
        float terrain_scale{1.f};
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
    LevelGameProfile profile_{LevelGameProfile::Unknown};
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
    mutable std::array<std::vector<DetailBatch>, FrameContext::FramesInFlight> transparent_detail_batches_;
    struct TransparentDetailDraw { uint8_t model; uint32_t first, count; };
    mutable std::array<std::vector<TransparentDetailDraw>, FrameContext::FramesInFlight> transparent_detail_draws_;
    std::vector<LevelVisual> visuals_;
    std::vector<uint8_t> visual_phases_; // opaque bit 1, forward bit 2, secondary opaque bit 4
    std::vector<uint32_t> roots_;
    std::vector<LevelSector> sectors_;
    std::vector<LevelPortal> portals_;
    std::vector<std::unique_ptr<VulkanVisual>> visual_objects_;
    std::unordered_map<const IRenderVisual*, uint32_t> visual_indices_;
    uint64_t revision_{};
    bool has_water_{};
    mutable ResourceIssueCounter draw_failures_;
};
}
