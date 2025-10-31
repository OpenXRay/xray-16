#pragma once

namespace xray::render::ng {

// Forward declarations
class ResourceManager;

// Handle types (will be defined later)
struct PipelineStateHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

struct BufferHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

struct TextureHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

// Render pass description
struct RenderPassDesc {
    nvrhi::TextureHandle renderTargets[8] = {};
    u32 numRenderTargets = 0;
    nvrhi::TextureHandle depthStencil = nullptr;

    struct ClearValue {
        float color[4] = {0, 0, 0, 0};
        float depth = 1.0f;
        u8 stencil = 0;
    } clearValue;

    bool clearColor = false;
    bool clearDepth = false;
    bool clearStencil = false;
};

// Viewport
struct Viewport {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

// Scissor rect
struct Rect {
    i32 x = 0;
    i32 y = 0;
    u32 width = 0;
    u32 height = 0;
};

/**
 * Modern command recording API
 *
 * Design principles:
 * - Explicit state management
 * - Minimal overhead
 * - Clear, readable API
 * - Future-proof for DX12/Vulkan
 */
class RenderContext {
public:
    RenderContext(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);
    ~RenderContext();

    // ═══════════════════════════════════════════════════════
    //  RENDER PASS MANAGEMENT
    // ═══════════════════════════════════════════════════════

    void BeginRenderPass(const RenderPassDesc& desc);
    void EndRenderPass();

    // ═══════════════════════════════════════════════════════
    //  PIPELINE STATE
    // ═══════════════════════════════════════════════════════

    void SetPipeline(PipelineStateHandle pso);
    void SetPipeline(nvrhi::IGraphicsPipeline* pipeline);  // Direct NVRHI (temporary)

    // ═══════════════════════════════════════════════════════
    //  VIEWPORT & SCISSOR
    // ═══════════════════════════════════════════════════════

    void SetViewport(const Viewport& viewport);
    void SetViewport(float x, float y, float width, float height);
    void SetScissor(const Rect& scissor);

    // ═══════════════════════════════════════════════════════
    //  VERTEX & INDEX BUFFERS
    // ═══════════════════════════════════════════════════════

    void SetVertexBuffer(u32 slot, BufferHandle buffer, u64 offset = 0);
    void SetVertexBuffer(u32 slot, nvrhi::IBuffer* buffer, u64 offset = 0);  // Direct

    void SetIndexBuffer(BufferHandle buffer, nvrhi::Format format, u64 offset = 0);
    void SetIndexBuffer(nvrhi::IBuffer* buffer, nvrhi::Format format, u64 offset = 0);

    // ═══════════════════════════════════════════════════════
    //  TEXTURES & SAMPLERS
    // ═══════════════════════════════════════════════════════

    void SetTexture(u32 slot, TextureHandle texture);
    void SetTexture(u32 slot, nvrhi::ITexture* texture);  // Direct

    void SetSampler(u32 slot, nvrhi::ISampler* sampler);

    // ═══════════════════════════════════════════════════════
    //  CONSTANT BUFFERS
    // ═══════════════════════════════════════════════════════

    void SetConstantBuffer(u32 slot, BufferHandle buffer);
    void SetConstantBuffer(u32 slot, nvrhi::IBuffer* buffer);  // Direct

    // ═══════════════════════════════════════════════════════
    //  DRAW CALLS
    // ═══════════════════════════════════════════════════════

    void Draw(u32 vertexCount, u32 startVertex = 0);
    void DrawIndexed(u32 indexCount, u32 startIndex = 0, i32 baseVertex = 0);
    void DrawInstanced(u32 vertexCount, u32 instanceCount,
                      u32 startVertex = 0, u32 startInstance = 0);
    void DrawIndexedInstanced(u32 indexCount, u32 instanceCount,
                             u32 startIndex = 0, i32 baseVertex = 0,
                             u32 startInstance = 0);

    // ═══════════════════════════════════════════════════════
    //  CLEAR OPERATIONS
    // ═══════════════════════════════════════════════════════

    void ClearRenderTarget(nvrhi::ITexture* rt, const float color[4]);
    void ClearDepthStencil(nvrhi::ITexture* ds, float depth, u8 stencil);

    // ═══════════════════════════════════════════════════════
    //  STATE QUERY
    // ═══════════════════════════════════════════════════════

    bool IsInRenderPass() const { return m_inRenderPass; }

    // ═══════════════════════════════════════════════════════
    //  INTERNAL
    // ═══════════════════════════════════════════════════════

    nvrhi::ICommandList* GetCommandList() const { return m_commandList; }

private:
    nvrhi::IDevice* m_device;
    nvrhi::ICommandList* m_commandList;
    ResourceManager* m_resourceManager = nullptr;  // Set later

    // State tracking
    bool m_inRenderPass = false;
    RenderPassDesc m_currentRenderPass;

    // Prevent copying
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
};

} // namespace xray::render::ng
