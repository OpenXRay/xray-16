#pragma once

#include "RCShader.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {

// Forward declarations
class RenderDevice;

// ═══════════════════════════════════════════════════
//  VERTEX INPUT LAYOUT
// ═══════════════════════════════════════════════════

struct VertexAttribute {
    const char* semanticName;
    u32 semanticIndex = 0;
    nvrhi::Format format;
    u32 offset;
    u32 bufferIndex = 0;
    bool isInstanced = false;
    u32 elementStride = 0;  // Stride of the vertex buffer this attribute belongs to
};

// ═══════════════════════════════════════════════════
//  RASTERIZER STATE
// ═══════════════════════════════════════════════════

enum class CullMode {
    None,
    Front,
    Back
};

enum class FillMode {
    Solid,
    Wireframe
};

struct RasterizerState {
    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    bool frontCounterClockwise = false;
    bool depthClipEnable = true;
    bool scissorEnable = false;
    bool multisampleEnable = false;
    bool antialiasedLineEnable = false;

    int depthBias = 0;
    float depthBiasClamp = 0.0f;
    float slopeScaledDepthBias = 0.0f;

    // Conservative rasterization (DX12 feature)
    bool conservativeRaster = false;
};

// ═══════════════════════════════════════════════════
//  BLEND STATE
// ═══════════════════════════════════════════════════

enum class BlendFactor {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstColor,
    InvDstColor,
    DstAlpha,
    InvDstAlpha,
    SrcAlphaSat,
    BlendFactor,
    InvBlendFactor
};

enum class BlendOp {
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

enum class ColorWriteMask : u8 {
    None = 0,
    Red = 1 << 0,
    Green = 1 << 1,
    Blue = 1 << 2,
    Alpha = 1 << 3,
    All = Red | Green | Blue | Alpha
};

// Bitwise operators for ColorWriteMask
inline ColorWriteMask operator|(ColorWriteMask a, ColorWriteMask b) {
    return static_cast<ColorWriteMask>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline ColorWriteMask operator&(ColorWriteMask a, ColorWriteMask b) {
    return static_cast<ColorWriteMask>(static_cast<u8>(a) & static_cast<u8>(b));
}

struct RenderTargetBlendState {
    bool blendEnable = false;

    BlendFactor srcBlend = BlendFactor::One;
    BlendFactor dstBlend = BlendFactor::Zero;
    BlendOp blendOp = BlendOp::Add;

    BlendFactor srcBlendAlpha = BlendFactor::One;
    BlendFactor dstBlendAlpha = BlendFactor::Zero;
    BlendOp blendOpAlpha = BlendOp::Add;

    ColorWriteMask writeMask = ColorWriteMask::All;
};

struct BlendState {
    RenderTargetBlendState renderTargets[8];
    bool alphaToCoverageEnable = false;
    bool independentBlendEnable = false;  // Different blend per RT
};

// ═══════════════════════════════════════════════════
//  DEPTH/STENCIL STATE
// ═══════════════════════════════════════════════════

enum class ComparisonFunc {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class StencilOp {
    Keep,
    Zero,
    Replace,
    IncrementSaturate,
    DecrementSaturate,
    Invert,
    Increment,
    Decrement
};

struct StencilOpState {
    StencilOp failOp = StencilOp::Keep;
    StencilOp depthFailOp = StencilOp::Keep;
    StencilOp passOp = StencilOp::Keep;
    ComparisonFunc compareFunc = ComparisonFunc::Always;
};

struct DepthStencilState {
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    ComparisonFunc depthFunc = ComparisonFunc::Greater;

    bool stencilEnable = false;
    u8 stencilReadMask = 0xFF;
    u8 stencilWriteMask = 0xFF;

    StencilOpState frontFace;
    StencilOpState backFace;
};

// ═══════════════════════════════════════════════════
//  PRIMITIVE TOPOLOGY
// ═══════════════════════════════════════════════════

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    PatchList  // For tessellation
};

// ═══════════════════════════════════════════════════
//  PIPELINE STATE DESCRIPTOR
// ═══════════════════════════════════════════════════

struct PipelineStateDesc {
    // ─── Shaders (Direct NVRHI - no wrapper!) ───
    nvrhi::IShader* vertexShader = nullptr;
    nvrhi::IShader* pixelShader = nullptr;
    nvrhi::IShader* geometryShader = nullptr;
    nvrhi::IShader* hullShader = nullptr;     // Tessellation control
    nvrhi::IShader* domainShader = nullptr;   // Tessellation evaluation

    // ─── Vertex Input ───
    xr_vector<VertexAttribute> vertexAttributes;

    // ─── Binding Layouts (Root Signature) ───
    xr_vector<nvrhi::BindingLayoutHandle> bindingLayouts;

    // ─── Fixed Function State ───
    RasterizerState rasterizerState;
    BlendState blendState;
    DepthStencilState depthStencilState;

    // ─── Primitive Topology ───
    PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;
    u32 patchControlPoints = 0;  // For tessellation

    // ─── Render Target Formats ───
    nvrhi::Format renderTargetFormats[8] = { nvrhi::Format::UNKNOWN };
    nvrhi::Format depthStencilFormat = nvrhi::Format::UNKNOWN;
    u32 renderTargetCount = 1;

    // ─── Multisampling ───
    u32 sampleCount = 1;
    u32 sampleQuality = 0;

    // ─── Debug ───
    shared_str debugName;

    // ─── Hash for Caching ───
    u64 ComputeHash() const;
};

// ═══════════════════════════════════════════════════
//  COMPILED PIPELINE STATE (Immutable)
// ═══════════════════════════════════════════════════

class PipelineState {
public:
    PipelineState(const PipelineStateDesc& desc,
                  nvrhi::GraphicsPipelineHandle nvrhiPipeline);

    const PipelineStateDesc& GetDesc() const { return m_desc; }
    nvrhi::IGraphicsPipeline* GetNativePipeline() const {
        return m_nvrhiPipeline.Get();
    }

    u64 GetHash() const { return m_hash; }

private:
    PipelineStateDesc m_desc;
    nvrhi::GraphicsPipelineHandle m_nvrhiPipeline;
    u64 m_hash;
};

// ═══════════════════════════════════════════════════
//  PIPELINE STATE CACHE
// ═══════════════════════════════════════════════════

class PipelineStateCache {
public:
    PipelineStateCache(RenderDevice* device);
    ~PipelineStateCache();

    // Get or create PSO (thread-safe)
    PipelineState* GetOrCreate(const PipelineStateDesc& desc);

    // Statistics
    struct Stats {
        u32 psoCount = 0;
        u32 cacheHits = 0;
        u32 cacheMisses = 0;

        float GetHitRate() const {
            u32 total = cacheHits + cacheMisses;
            return total > 0 ? (float)cacheHits / total : 0.0f;
        }
    };

    const Stats& GetStats() const { return m_stats; }
    void ResetStats();

    // Clear cache (e.g. on shader reload)
    void Clear();

private:
    RenderDevice* m_device;

    // Cache: hash -> PSO
    xr_map<u64, xr_unique_ptr<PipelineState>> m_cache;

    // Thread safety
    mutable Lock m_cacheLock;

    // Statistics
    Stats m_stats;

    // Create new PSO
    PipelineState* CreatePipelineState(const PipelineStateDesc& desc);

    // Conversion helpers (X-Ray enums -> NVRHI enums)
    static nvrhi::RasterCullMode ConvertCullMode(CullMode mode);
    static nvrhi::RasterFillMode ConvertFillMode(FillMode mode);
    static nvrhi::BlendFactor ConvertBlendFactor(BlendFactor factor);
    static nvrhi::BlendOp ConvertBlendOp(BlendOp op);
    static nvrhi::ComparisonFunc ConvertComparisonFunc(ComparisonFunc func);
    static nvrhi::StencilOp ConvertStencilOp(StencilOp op);
    static nvrhi::PrimitiveType ConvertPrimitiveTopology(PrimitiveTopology topology);
};

} // namespace xray::render::fg
