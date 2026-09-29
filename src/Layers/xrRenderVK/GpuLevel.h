#pragma once

#include "BufferUpload.h"
#include "DeferredPass.h"
#include "EngineLevelModels.h"
#include "GameTextureFactory.h"

namespace xray::render::vulkan
{
// GPU ownership of the supported static level subset. A failed load leaves
// the previous level intact; no partially uploaded scene is exposed.
class GpuLevel
{
public:
    GpuLevel() = default;
    ~GpuLevel() { destroy(); }
    GpuLevel(const GpuLevel&) = delete;
    GpuLevel& operator=(const GpuLevel&) = delete;

    bool load(IReader& level, VkDevice device, VkQueue queue, VkCommandPool pool,
        const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
        GameTextureFactory& textures, DeferredPass& pass, std::string& error);
    bool record(const FrameRecordingContext& frame, const DeferredPass& pass,
        const float (&mvp)[16]) const;
    size_t model_count() const { return meshes_.size(); }
    void destroy();

private:
    struct Mesh
    {
        BufferResource vertices, indices;
        uint32_t index_count{};
        VkDescriptorSet material{};
    };
    VkDevice device_{};
    VkCommandPool pool_{};
    BufferUploadDispatch upload_{};
    std::vector<PendingBufferUpload> pending_;
    std::vector<Mesh> meshes_;
};
}
