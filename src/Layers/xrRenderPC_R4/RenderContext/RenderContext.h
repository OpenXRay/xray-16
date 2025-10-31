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

struct BindingLayoutHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

struct BindingSetHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

struct BindingLayoutItem {
    nvrhi::ResourceType type;
    u32 slot;
    u32 size;

    BindingLayoutItem()
        : type(nvrhi::ResourceType::None)
        , slot(0)
        , size(1)
    {}
};

struct BindingLayoutDesc {
    BindingLayoutItem items[32];
    u32 numItems = 0;
    nvrhi::ShaderType visibility = nvrhi::ShaderType::All;
    u32 registerSpace = 0;

    BindingLayoutDesc() : numItems(0) {}

    void AddItem(nvrhi::ResourceType type, u32 slot, u32 size = 1) {
        VERIFY(numItems < 32);
        items[numItems].type = type;
        items[numItems].slot = slot;
        items[numItems].size = size;
        numItems++;
    }

    void SetVisibility(nvrhi::ShaderType stages) {
        visibility = stages;
    }
};

struct BindingSetItem {
    u32 slot;
    u32 arrayElement = 0;
    nvrhi::ResourceType type;
    nvrhi::IResource* resource = nullptr;
    nvrhi::Format format = nvrhi::Format::UNKNOWN;
    nvrhi::TextureDimension dimension = nvrhi::TextureDimension::Unknown;

    BindingSetItem() : slot(0), type(nvrhi::ResourceType::None) {}
};

struct BindingSetDesc {
    nvrhi::IBindingLayout* layout = nullptr;
    BindingSetItem items[32];
    u32 numItems = 0;

    BindingSetDesc() : numItems(0) {}

    void AddTexture(u32 slot, nvrhi::ITexture* texture) {
        VERIFY(numItems < 32);
        items[numItems].slot = slot;
        items[numItems].type = nvrhi::ResourceType::Texture_SRV;
        items[numItems].resource = texture;
        numItems++;
    }

    void AddSampler(u32 slot, nvrhi::ISampler* sampler) {
        VERIFY(numItems < 32);
        items[numItems].slot = slot;
        items[numItems].type = nvrhi::ResourceType::Sampler;
        items[numItems].resource = sampler;
        numItems++;
    }

    void AddConstantBuffer(u32 slot, nvrhi::IBuffer* buffer) {
        VERIFY(numItems < 32);
        items[numItems].slot = slot;
        items[numItems].type = nvrhi::ResourceType::ConstantBuffer;
        items[numItems].resource = buffer;
        numItems++;
    }
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

    nvrhi::BindingLayoutHandle CreateBindingLayout(const BindingLayoutDesc& desc);
    BindingLayoutHandle CreateBindingLayout(const BindingLayoutDesc& desc, bool useHandle);

    nvrhi::BindingSetHandle CreateBindingSet(const BindingSetDesc& desc);
    BindingSetHandle CreateBindingSet(const BindingSetDesc& desc, bool useHandle);

    void SetBindingSet(u32 slot, nvrhi::IBindingSet* bindingSet);
    void SetBindingSet(u32 slot, BindingSetHandle bindingSet);

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
