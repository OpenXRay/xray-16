#pragma once

#include "UIGeometryBatch.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class MaterialCache;
    struct MaterialPSO;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::ui
{
using namespace xray::render::RENDER_NAMESPACE;

class NVRHIUIRenderer
{
public:
    NVRHIUIRenderer();
    ~NVRHIUIRenderer();

    void Initialize(fg::RenderDevice* device, MaterialCache* matCache);
    void Shutdown();

    void RenderBatches(
        nvrhi::ICommandList* commandList,
        const xr_vector<UIGeometryBatch>& batches,
        nvrhi::IFramebuffer* framebuffer,
        u32 screenWidth,
        u32 screenHeight
    );

private:
    fg::RenderDevice* m_device{nullptr};
    MaterialCache* m_matCache{nullptr};

    nvrhi::BufferHandle m_vertexBuffer;
    nvrhi::BufferHandle m_indexBuffer;

    nvrhi::BufferHandle m_constantBuffer;

    size_t m_vertexBufferSize{0};
    size_t m_indexBufferSize{0};

    bool CreateBuffers();

    void EnsureBufferCapacity(size_t vertexCount, size_t indexCount);
    void UploadBatchGeometry(nvrhi::ICommandList* commandList, const UIGeometryBatch& batch,
                            u32& vertexOffset, u32& indexOffset);

    void RenderBatchesWithShader(
        nvrhi::ICommandList* commandList,
        const xr_vector<const UIGeometryBatch*>& batches,
        MaterialPSO* pso,
        nvrhi::IFramebuffer* framebuffer,
        u32 screenWidth,
        u32 screenHeight);

    bool m_initialized{false};
};

}
