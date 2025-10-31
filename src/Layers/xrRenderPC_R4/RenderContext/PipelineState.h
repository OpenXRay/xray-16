#pragma once

#include "RCShader.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::ng {

class RenderDevice;

struct VertexAttribute {
    const char* semanticName;
    u32 semanticIndex = 0;
    nvrhi::Format format;
    u32 offset;
    u32 bufferIndex = 0;
    bool isInstanced = false;
};

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

    bool conservativeRaster = false;
};

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
    bool independentBlendEnable = false;
};

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
    ComparisonFunc depthFunc = ComparisonFunc::Less;

    bool stencilEnable = false;
    u8 stencilReadMask = 0xFF;
    u8 stencilWriteMask = 0xFF;

    StencilOpState frontFace;
    StencilOpState backFace;
};

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    PatchList
};

struct PipelineStateDesc {
    RCShader* vertexShader = nullptr;
    RCShader* pixelShader = nullptr;
    RCShader* geometryShader = nullptr;
    RCShader* hullShader = nullptr;
    RCShader* domainShader = nullptr;

    xr_vector<VertexAttribute> vertexAttributes;

    RasterizerState rasterizerState;
    BlendState blendState;
    DepthStencilState depthStencilState;

    PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;
    u32 patchControlPoints = 0;

    nvrhi::Format renderTargetFormats[8] = { nvrhi::Format::UNKNOWN };
    nvrhi::Format depthStencilFormat = nvrhi::Format::UNKNOWN;
    u32 renderTargetCount = 1;

    u32 sampleCount = 1;
    u32 sampleQuality = 0;

    shared_str debugName;

    u64 ComputeHash() const;
};

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

class PipelineStateCache {
public:
    PipelineStateCache(RenderDevice* device);
    ~PipelineStateCache();

    PipelineState* GetOrCreate(const PipelineStateDesc& desc);

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

    void Clear();

private:
    RenderDevice* m_device;

    xr_map<u64, xr_unique_ptr<PipelineState>> m_cache;

    mutable Lock m_cacheLock;

    Stats m_stats;

    PipelineState* CreatePipelineState(const PipelineStateDesc& desc);

    static nvrhi::RasterCullMode ConvertCullMode(CullMode mode);
    static nvrhi::RasterFillMode ConvertFillMode(FillMode mode);
    static nvrhi::BlendFactor ConvertBlendFactor(BlendFactor factor);
    static nvrhi::BlendOp ConvertBlendOp(BlendOp op);
    static nvrhi::ComparisonFunc ConvertComparisonFunc(ComparisonFunc func);
    static nvrhi::StencilOp ConvertStencilOp(StencilOp op);
    static nvrhi::PrimitiveType ConvertPrimitiveTopology(PrimitiveTopology topology);
};

}
