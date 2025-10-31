#pragma once

namespace xray::render::ng {

class ResourceManager;

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

struct Viewport {
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;

    Viewport() : x(0), y(0), width(0), height(0), minDepth(0.0f), maxDepth(1.0f) {}
};

struct Rect {
    int x;
    int y;
    u32 width;
    u32 height;

    Rect() : x(0), y(0), width(0), height(0) {}
};

class RenderContext {
public:
    RenderContext(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);
    ~RenderContext();

    void BeginRenderPass(const RenderPassDesc& desc);
    void EndRenderPass();

    void SetPipeline(PipelineStateHandle pso);
    void SetPipeline(nvrhi::IGraphicsPipeline* pipeline);

    void SetViewport(const Viewport& viewport);
    void SetViewport(float x, float y, float width, float height);
    void SetScissor(const Rect& scissor);

    void SetVertexBuffer(u32 slot, BufferHandle buffer, u64 offset = 0);
    void SetVertexBuffer(u32 slot, nvrhi::IBuffer* buffer, u64 offset = 0);

    void SetIndexBuffer(BufferHandle buffer, nvrhi::Format format, u64 offset = 0);
    void SetIndexBuffer(nvrhi::IBuffer* buffer, nvrhi::Format format, u64 offset = 0);

    void SetTexture(u32 slot, TextureHandle texture);
    void SetTexture(u32 slot, nvrhi::ITexture* texture);

    void SetSampler(u32 slot, nvrhi::ISampler* sampler);

    void SetConstantBuffer(u32 slot, BufferHandle buffer);
    void SetConstantBuffer(u32 slot, nvrhi::IBuffer* buffer);

    void Draw(u32 vertexCount, u32 startVertex = 0);
    void DrawIndexed(u32 indexCount, u32 startIndex = 0, int baseVertex = 0);
    void DrawInstanced(u32 vertexCount, u32 instanceCount,
                      u32 startVertex = 0, u32 startInstance = 0);
    void DrawIndexedInstanced(u32 indexCount, u32 instanceCount,
                             u32 startIndex = 0, int baseVertex = 0,
                             u32 startInstance = 0);

    void ClearRenderTarget(nvrhi::ITexture* rt, const float color[4]);
    void ClearDepthStencil(nvrhi::ITexture* ds, float depth, u8 stencil);

    bool IsInRenderPass() const { return m_inRenderPass; }

    nvrhi::ICommandList* GetCommandList() const { return m_commandList; }

private:
    nvrhi::IDevice* m_device;
    nvrhi::ICommandList* m_commandList;
    ResourceManager* m_resourceManager = nullptr;

    bool m_inRenderPass = false;
    RenderPassDesc m_currentRenderPass;
    nvrhi::FramebufferHandle m_currentFramebuffer;
    nvrhi::GraphicsState m_currentState;

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
};

}
