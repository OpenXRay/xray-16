#pragma once

#include "BufferUpload.h"
#include "DeferredPass.h"
#include "EngineLevelModels.h"
#include "GameTextureFactory.h"

#include <array>
#include <unordered_map>

class IKinematics;
namespace xray::render::vulkan
{
// A standalone OGF model. Vertex buffers are per-flight-slot because an
// animated pose may change while an earlier frame still reads its vertices.
class GpuModel
{
public:
    ~GpuModel() { destroy(); }
    GpuModel(const GpuModel&) = delete;
    GpuModel& operator=(const GpuModel&) = delete;
    GpuModel() = default;

    bool load(const char* name, IReader* source, VkDevice device, VkQueue queue,
        VkCommandPool pool, const VkPhysicalDeviceMemoryProperties& memory,
        const BufferUploadDispatch& upload, GameTextureFactory& textures,
        DeferredPass& pass, std::string& error);
    bool load_record(const VisualRecord& record, const std::string& inherited_texture,
        VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
        GameTextureFactory& textures, DeferredPass& pass, std::string& error);
    bool load_decoded(ModelGeometry&& geometry, VkDevice device, VkQueue queue,
        VkCommandPool pool, const VkPhysicalDeviceMemoryProperties& memory,
        const BufferUploadDispatch& upload, GameTextureFactory& textures,
        DeferredPass& pass, std::string& error)
    {
        return load_geometry(std::move(geometry), device, queue, pool, memory,
            upload, textures, pass, error);
    }
    // Call from the acquired frame's geometry recorder. Pose matrices have
    // the same layout as Fmatrix and include inverse bind transforms.
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass, const float (&mvp)[16], const float* pose, size_t bones, std::string& error,
        GeometryPhase phase = GeometryPhase::OpaqueAndAlphaTest, float lod = 1.f, const IKinematics* instance = nullptr);
    bool record_animated(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16], IKinematics& skeleton, std::string& error,
        GeometryPhase phase = GeometryPhase::OpaqueAndAlphaTest, float lod = 1.f);
    void destroy(); // Caller waits for all submitted frames first.
    void release_instance(const IKinematics* skeleton); // Caller waits for submitted frames first.

private:
    bool load_geometry(ModelGeometry&& decoded, VkDevice device, VkQueue queue,
        VkCommandPool pool, const VkPhysicalDeviceMemoryProperties& memory,
        const BufferUploadDispatch& upload, GameTextureFactory& textures,
        DeferredPass& pass, std::string& error);
    struct Mesh
    {
        struct PoseInstance
        {
            std::array<BufferResource, FrameContext::FramesInFlight> buffers;
            std::array<VkDescriptorSet, FrameContext::FramesInFlight> descriptors{};
        };
        ModelGeometry geometry;
        BufferResource skinned_vertices;
        std::array<BufferResource, FrameContext::FramesInFlight> vertices;
        std::unordered_map<const IKinematics*, std::array<BufferResource, FrameContext::FramesInFlight>> instance_vertices;
        std::unordered_map<const IKinematics*, PoseInstance> poses;
        BufferResource indices;
        VkDescriptorSet material{};
        bool gpu_skinning{};
        uint16_t max_bone_index{};
        size_t fast_index = SIZE_MAX;
        bool fast_variant = false;
    };
    bool add_meshes(ModelGeometry&& geometry, const std::string& inherited_texture,
        VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory,
        const BufferUploadDispatch& upload, GameTextureFactory& textures,
        DeferredPass& pass, std::string& error);

    VkDevice device_{};
    VkCommandPool pool_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    BufferUploadDispatch upload_{};
    GameTextureFactory* textures_{};
    DeferredPass* pass_{};
    std::vector<PendingBufferUpload> pending_;
    std::vector<Mesh> meshes_;
};
}
