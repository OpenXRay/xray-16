#pragma once

#include "ResourceHandle.h"

namespace xray::render::ng {

class ResourceManager;

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

class RenderDevice;

class RenderContext {
public:
    RenderContext(RenderDevice* device, nvrhi::ICommandList* commandList);
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

    struct RenderStats {
        u32 numDrawCalls = 0;
        u32 numDrawIndexedCalls = 0;
        u32 numDrawInstancedCalls = 0;
        u32 numDrawIndexedInstancedCalls = 0;

        u32 numPipelineChanges = 0;
        u32 numViewportChanges = 0;
        u32 numScissorChanges = 0;
        u32 numVertexBufferChanges = 0;
        u32 numIndexBufferChanges = 0;
        u32 numBindingSetChanges = 0;

        u32 numRedundantPipeline = 0;
        u32 numRedundantViewport = 0;
        u32 numRedundantVertexBuffer = 0;
        u32 numRedundantIndexBuffer = 0;
        u32 numRedundantBindingSet = 0;

        void Reset() {
            numDrawCalls = 0;
            numDrawIndexedCalls = 0;
            numDrawInstancedCalls = 0;
            numDrawIndexedInstancedCalls = 0;
            numPipelineChanges = 0;
            numViewportChanges = 0;
            numScissorChanges = 0;
            numVertexBufferChanges = 0;
            numIndexBufferChanges = 0;
            numBindingSetChanges = 0;
            numRedundantPipeline = 0;
            numRedundantViewport = 0;
            numRedundantVertexBuffer = 0;
            numRedundantIndexBuffer = 0;
            numRedundantBindingSet = 0;
        }
    };

    const RenderStats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats.Reset(); }

    bool IsInRenderPass() const { return m_inRenderPass; }

    nvrhi::ICommandList* GetCommandList() const { return m_commandList; }

private:
    RenderDevice* m_device;
    nvrhi::ICommandList* m_commandList;
    ResourceManager* m_resourceManager = nullptr;

    bool m_inRenderPass = false;
    RenderPassDesc m_currentRenderPass;
    nvrhi::FramebufferHandle m_currentFramebuffer;
    nvrhi::GraphicsState m_currentState;

    struct StateCache {
        nvrhi::IGraphicsPipeline* pipeline = nullptr;
        nvrhi::IBuffer* vertexBuffers[8] = {};
        u64 vertexBufferOffsets[8] = {};
        nvrhi::IBuffer* indexBuffer = nullptr;
        nvrhi::Format indexBufferFormat = nvrhi::Format::UNKNOWN;
        u64 indexBufferOffset = 0;
        nvrhi::IBindingSet* bindingSets[6] = {};
        Viewport viewport;
        Rect scissor;
        bool viewportSet = false;
        bool scissorSet = false;

        void Reset() {
            pipeline = nullptr;
            for (int i = 0; i < 8; i++) {
                vertexBuffers[i] = nullptr;
                vertexBufferOffsets[i] = 0;
            }
            indexBuffer = nullptr;
            indexBufferFormat = nvrhi::Format::UNKNOWN;
            indexBufferOffset = 0;
            for (int i = 0; i < 6; i++) {
                bindingSets[i] = nullptr;
            }
            viewportSet = false;
            scissorSet = false;
        }
    } m_stateCache;

    RenderStats m_stats;

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
};

}
