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

struct BindingLayoutHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

struct BindingSetHandle {
    u32 index = 0;
    bool IsValid() const { return index != 0; }
};

// Binding layout description (what resources a shader expects)
struct BindingLayoutItem {
    nvrhi::ResourceType type;  // Texture, Sampler, ConstantBuffer, etc.
    u32 slot;                  // Binding slot in shader
    u32 size;                  // Array size (1 for single resource)

    BindingLayoutItem()
        : type(nvrhi::ResourceType::None)
        , slot(0)
        , size(1)
    {}
};

struct BindingLayoutDesc {
    BindingLayoutItem items[32];       // Max 32 bindings per layout
    u32 numItems = 0;
    nvrhi::ShaderType visibility = nvrhi::ShaderType::All;  // Which shader stages can see this layout
    u32 registerSpace = 0;             // DX12/Vulkan register space

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

// Binding set description (actual resources to bind)
struct BindingSetItem {
    u32 slot;                              // Must match layout slot
    u32 arrayElement = 0;                  // Index in binding array (usually 0)
    nvrhi::ResourceType type;              // Must match layout type
    nvrhi::IResource* resource = nullptr;  // Texture, Buffer, or Sampler
    nvrhi::Format format = nvrhi::Format::UNKNOWN;           // For typed buffers/textures
    nvrhi::TextureDimension dimension = nvrhi::TextureDimension::Unknown;  // For textures

    BindingSetItem() : slot(0), type(nvrhi::ResourceType::None) {}
};

struct BindingSetDesc {
    nvrhi::IBindingLayout* layout = nullptr;  // Binding layout to use
    BindingSetItem items[32];                 // Max 32 resources
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
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;

    Viewport() : x(0), y(0), width(0), height(0), minDepth(0.0f), maxDepth(1.0f) {}
};

// Scissor rect
struct Rect {
    int x;
    int y;
    u32 width;
    u32 height;

    Rect() : x(0), y(0), width(0), height(0) {}
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
    //  BINDING LAYOUTS & DESCRIPTOR SETS
    // ═══════════════════════════════════════════════════════

    // Create binding layout (describes what resources shader expects)
    nvrhi::BindingLayoutHandle CreateBindingLayout(const BindingLayoutDesc& desc);
    BindingLayoutHandle CreateBindingLayout(const BindingLayoutDesc& desc, bool useHandle);  // Future

    // Create binding set (actual resources to bind)
    nvrhi::BindingSetHandle CreateBindingSet(const BindingSetDesc& desc);
    BindingSetHandle CreateBindingSet(const BindingSetDesc& desc, bool useHandle);  // Future

    // Bind resources to shader (sets must match pipeline's expected layouts)
    void SetBindingSet(u32 slot, nvrhi::IBindingSet* bindingSet);
    void SetBindingSet(u32 slot, BindingSetHandle bindingSet);  // Future

    // ═══════════════════════════════════════════════════════
    //  DRAW CALLS
    // ═══════════════════════════════════════════════════════

    void Draw(u32 vertexCount, u32 startVertex = 0);
    void DrawIndexed(u32 indexCount, u32 startIndex = 0, int baseVertex = 0);
    void DrawInstanced(u32 vertexCount, u32 instanceCount,
                      u32 startVertex = 0, u32 startInstance = 0);
    void DrawIndexedInstanced(u32 indexCount, u32 instanceCount,
                             u32 startIndex = 0, int baseVertex = 0,
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
    nvrhi::FramebufferHandle m_currentFramebuffer;
    nvrhi::GraphicsState m_currentState;  // Track current graphics state

    // Prevent copying
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
};

} // namespace xray::render::ng
