#pragma once

#include "BufferUpload.h"
#include "DeferredPass.h"
#include "EngineLevelModels.h"
#include "GameTextureFactory.h"

#include <array>

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
    // Call from the acquired frame's geometry recorder. Pose matrices have
    // the same layout as Fmatrix and include inverse bind transforms.
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16], const float* pose, size_t bones,
        std::string& error,
        GeometryPhase phase = GeometryPhase::OpaqueAndAlphaTest);
    bool record_animated(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16], IKinematics& skeleton, std::string& error,
        GeometryPhase phase = GeometryPhase::OpaqueAndAlphaTest);
    void destroy(); // Caller waits for all submitted frames first.

private:
    struct Mesh
    {
        ModelGeometry geometry;
        std::array<BufferResource, FrameContext::FramesInFlight> vertices;
        BufferResource indices;
        VkDescriptorSet material{};
    };
    bool add_meshes(ModelGeometry&& geometry, const std::string& inherited_texture,
        VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory,
        const BufferUploadDispatch& upload, GameTextureFactory& textures,
        DeferredPass& pass, std::string& error);

    VkDevice device_{};
    VkCommandPool pool_{};
    BufferUploadDispatch upload_{};
    GameTextureFactory* textures_{};
    DeferredPass* pass_{};
    std::vector<PendingBufferUpload> pending_;
    std::vector<Mesh> meshes_;
};
}
