// xrRender/NVRHIUIRenderer.h
#pragma once

#include "UIGeometryBatch.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include <nvrhi/nvrhi.h>

// Forward declarations
namespace xray::render {
    class MaterialCache;
    struct MaterialPSO;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::ui
{
using namespace xray::render::RENDER_NAMESPACE;  // For HW

// NVRHI-based UI renderer backend
// Uses MaterialCache directly to extract shader bytecode and bindings from ref_shader
class NVRHIUIRenderer
{
public:
    NVRHIUIRenderer();
    ~NVRHIUIRenderer();

    // Initialize the renderer with device and material cache
    void Initialize(ng::RenderDevice* device, MaterialCache* matCache);
    void Shutdown();

    // Render collected UI batches
    void RenderBatches(
        nvrhi::ICommandList* commandList,
        const xr_vector<UIGeometryBatch>& batches,
        nvrhi::IFramebuffer* framebuffer,
        u32 screenWidth,
        u32 screenHeight
    );

private:
    // Dependencies
    ng::RenderDevice* m_device{nullptr};
    MaterialCache* m_matCache{nullptr};

    // Shared geometry buffers (all batches share these)
    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;

    // UI constants buffer (screen size)
    nvrhi::BufferHandle m_constantBuffer;

    // Current buffer sizes
    size_t m_vertexBufferSize{0};
    size_t m_indexBufferSize{0};

    // Initialization helpers
    bool CreateBuffers();

    // Rendering helpers
    void EnsureBufferCapacity(size_t vertexCount, size_t indexCount);
    void UploadBatchGeometry(nvrhi::ICommandList* commandList, const UIGeometryBatch& batch,
                            u32& vertexOffset, u32& indexOffset);

    // Per-shader rendering (uses MaterialPSO from cache)
    void RenderBatchesWithShader(
        nvrhi::ICommandList* commandList,
        const xr_vector<const UIGeometryBatch*>& batches,
        MaterialPSO* pso,
        nvrhi::IFramebuffer* framebuffer,
        u32 screenWidth,
        u32 screenHeight);

    bool m_initialized{false};
};

} // namespace xray::render::ui
