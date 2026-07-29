#include "stdafx.h"
#include "PipelineState.h"
#include "RenderDevice.h"

namespace xray::render::fg {

// ═══════════════════════════════════════════════════
//  DESCRIPTOR HASH COMPUTATION
// ═══════════════════════════════════════════════════

u64 PipelineStateDesc::ComputeHash() const {
    // Use FNV-1a hash
    u64 hash = 0xcbf29ce484222325ull;  // FNV-1a offset basis
    const u64 fnvPrime = 0x100000001b3ull;

    auto hashValue = [&hash, fnvPrime](u64 value) {
        hash ^= value;
        hash *= fnvPrime;
    };

    // Hash shader pointers
    hashValue((u64)vertexShader);
    hashValue((u64)pixelShader);
    hashValue((u64)geometryShader);
    hashValue((u64)hullShader);
    hashValue((u64)domainShader);

    // Hash rasterizer state
    hashValue((u32)rasterizerState.cullMode);
    hashValue((u32)rasterizerState.fillMode);
    hashValue(rasterizerState.frontCounterClockwise ? 1 : 0);
    hashValue(rasterizerState.depthClipEnable ? 1 : 0);
    hashValue(rasterizerState.scissorEnable ? 1 : 0);
    hashValue(rasterizerState.depthBias);
    hashValue(*(u32*)&rasterizerState.depthBiasClamp);
    hashValue(*(u32*)&rasterizerState.slopeScaledDepthBias);

    // Hash blend state
    hashValue(blendState.alphaToCoverageEnable ? 1 : 0);
    hashValue(blendState.independentBlendEnable ? 1 : 0);
    for (u32 i = 0; i < renderTargetCount; i++) {
        const auto& rt = blendState.renderTargets[i];
        hashValue(rt.blendEnable ? 1 : 0);
        hashValue((u32)rt.srcBlend);
        hashValue((u32)rt.dstBlend);
        hashValue((u32)rt.blendOp);
        hashValue((u32)rt.srcBlendAlpha);
        hashValue((u32)rt.dstBlendAlpha);
        hashValue((u32)rt.blendOpAlpha);
        hashValue((u8)rt.writeMask);
    }

    // Hash depth/stencil state
    hashValue(depthStencilState.depthTestEnable ? 1 : 0);
    hashValue(depthStencilState.depthWriteEnable ? 1 : 0);
    hashValue((u32)depthStencilState.depthFunc);
    hashValue(depthStencilState.stencilEnable ? 1 : 0);
    hashValue(depthStencilState.stencilReadMask);
    hashValue(depthStencilState.stencilWriteMask);
    hashValue((u32)depthStencilState.frontFace.failOp);
    hashValue((u32)depthStencilState.frontFace.depthFailOp);
    hashValue((u32)depthStencilState.frontFace.passOp);
    hashValue((u32)depthStencilState.frontFace.compareFunc);

    // Hash vertex attributes
    for (const auto& attr : vertexAttributes) {
        // Hash semantic name
        const char* name = attr.semanticName;
        while (*name) {
            hashValue(*name++);
        }
        hashValue(attr.semanticIndex);
        hashValue((u32)attr.format);
        hashValue(attr.offset);
        hashValue(attr.bufferIndex);
        hashValue(attr.isInstanced ? 1 : 0);
    }

    // Hash topology
    hashValue((u32)primitiveTopology);
    hashValue(patchControlPoints);

    // Hash render target formats
    for (u32 i = 0; i < renderTargetCount; i++) {
        hashValue((u32)renderTargetFormats[i]);
    }
    hashValue((u32)depthStencilFormat);
    hashValue(renderTargetCount);

    // Hash multisampling
    hashValue(sampleCount);
    hashValue(sampleQuality);

    for (const auto& layout : bindingLayouts)
        hashValue((u64)layout.Get());

    return hash;
}

// ═══════════════════════════════════════════════════
//  PIPELINE STATE
// ═══════════════════════════════════════════════════

PipelineState::PipelineState(
    const PipelineStateDesc& desc,
    nvrhi::GraphicsPipelineHandle nvrhiPipeline)
    : m_desc(desc)
    , m_nvrhiPipeline(nvrhiPipeline)
    , m_hash(desc.ComputeHash())
{
}

// ═══════════════════════════════════════════════════
//  CONVERSION HELPERS
// ═══════════════════════════════════════════════════

nvrhi::RasterCullMode PipelineStateCache::ConvertCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None:  return nvrhi::RasterCullMode::None;
        case CullMode::Front: return nvrhi::RasterCullMode::Front;
        case CullMode::Back:  return nvrhi::RasterCullMode::Back;
        default: return nvrhi::RasterCullMode::Back;
    }
}

nvrhi::RasterFillMode PipelineStateCache::ConvertFillMode(FillMode mode) {
    switch (mode) {
        case FillMode::Solid:     return nvrhi::RasterFillMode::Fill;
        case FillMode::Wireframe: return nvrhi::RasterFillMode::Line;
        default: return nvrhi::RasterFillMode::Fill;
    }
}

nvrhi::BlendFactor PipelineStateCache::ConvertBlendFactor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero:          return nvrhi::BlendFactor::Zero;
        case BlendFactor::One:           return nvrhi::BlendFactor::One;
        case BlendFactor::SrcColor:      return nvrhi::BlendFactor::SrcColor;
        case BlendFactor::InvSrcColor:   return nvrhi::BlendFactor::InvSrcColor;
        case BlendFactor::SrcAlpha:      return nvrhi::BlendFactor::SrcAlpha;
        case BlendFactor::InvSrcAlpha:   return nvrhi::BlendFactor::InvSrcAlpha;
        case BlendFactor::DstColor:      return nvrhi::BlendFactor::DstColor;
        case BlendFactor::InvDstColor:   return nvrhi::BlendFactor::InvDstColor;
        case BlendFactor::DstAlpha:      return nvrhi::BlendFactor::DstAlpha;
        case BlendFactor::InvDstAlpha:   return nvrhi::BlendFactor::InvDstAlpha;
        case BlendFactor::SrcAlphaSat:   return nvrhi::BlendFactor::SrcAlphaSaturate;
        case BlendFactor::BlendFactor:   return nvrhi::BlendFactor::ConstantColor;
        case BlendFactor::InvBlendFactor: return nvrhi::BlendFactor::InvConstantColor;
        default: return nvrhi::BlendFactor::One;
    }
}

nvrhi::BlendOp PipelineStateCache::ConvertBlendOp(BlendOp op) {
    switch (op) {
        case BlendOp::Add:         return nvrhi::BlendOp::Add;
        case BlendOp::Subtract:    return nvrhi::BlendOp::Subtract;
        case BlendOp::RevSubtract: return nvrhi::BlendOp::ReverseSubtract;
        case BlendOp::Min:         return nvrhi::BlendOp::Min;
        case BlendOp::Max:         return nvrhi::BlendOp::Max;
        default: return nvrhi::BlendOp::Add;
    }
}

nvrhi::ComparisonFunc PipelineStateCache::ConvertComparisonFunc(ComparisonFunc func) {
    switch (func) {
        case ComparisonFunc::Never:        return nvrhi::ComparisonFunc::Never;
        case ComparisonFunc::Less:         return nvrhi::ComparisonFunc::Less;
        case ComparisonFunc::Equal:        return nvrhi::ComparisonFunc::Equal;
        case ComparisonFunc::LessEqual:    return nvrhi::ComparisonFunc::LessOrEqual;
        case ComparisonFunc::Greater:      return nvrhi::ComparisonFunc::Greater;
        case ComparisonFunc::NotEqual:     return nvrhi::ComparisonFunc::NotEqual;
        case ComparisonFunc::GreaterEqual: return nvrhi::ComparisonFunc::GreaterOrEqual;
        case ComparisonFunc::Always:       return nvrhi::ComparisonFunc::Always;
        default: return nvrhi::ComparisonFunc::Greater;
    }
}

nvrhi::StencilOp PipelineStateCache::ConvertStencilOp(StencilOp op) {
    switch (op) {
        case StencilOp::Keep:              return nvrhi::StencilOp::Keep;
        case StencilOp::Zero:              return nvrhi::StencilOp::Zero;
        case StencilOp::Replace:           return nvrhi::StencilOp::Replace;
        case StencilOp::IncrementSaturate: return nvrhi::StencilOp::IncrementAndClamp;
        case StencilOp::DecrementSaturate: return nvrhi::StencilOp::DecrementAndClamp;
        case StencilOp::Invert:            return nvrhi::StencilOp::Invert;
        case StencilOp::Increment:         return nvrhi::StencilOp::IncrementAndWrap;
        case StencilOp::Decrement:         return nvrhi::StencilOp::DecrementAndWrap;
        default: return nvrhi::StencilOp::Keep;
    }
}

nvrhi::PrimitiveType PipelineStateCache::ConvertPrimitiveTopology(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::PointList:     return nvrhi::PrimitiveType::PointList;
        case PrimitiveTopology::LineList:      return nvrhi::PrimitiveType::LineList;
        case PrimitiveTopology::LineStrip:     return nvrhi::PrimitiveType::LineStrip;
        case PrimitiveTopology::TriangleList:  return nvrhi::PrimitiveType::TriangleList;
        case PrimitiveTopology::TriangleStrip: return nvrhi::PrimitiveType::TriangleStrip;
        case PrimitiveTopology::PatchList:     return nvrhi::PrimitiveType::PatchList;
        default: return nvrhi::PrimitiveType::TriangleList;
    }
}

// ═══════════════════════════════════════════════════
//  PIPELINE STATE CACHE
// ═══════════════════════════════════════════════════

PipelineStateCache::PipelineStateCache(RenderDevice* device)
    : m_device(device)
{
    Msg("! [PipelineStateCache] Created");
}

PipelineStateCache::~PipelineStateCache() {
    Msg("! [PipelineStateCache] Statistics:");
    Msg("!   Total PSOs: %u", m_stats.psoCount);
    Msg("!   Cache Hits: %u", m_stats.cacheHits);
    Msg("!   Cache Misses: %u", m_stats.cacheMisses);
    Msg("!   Hit Rate: %.1f%%", m_stats.GetHitRate() * 100.0f);
}

PipelineState* PipelineStateCache::GetOrCreate(
    const PipelineStateDesc& desc) {

    u64 hash = desc.ComputeHash();

    // Try cache first (read lock)
    {
        ScopeLock lock(&m_cacheLock);
        auto it = m_cache.find(hash);
        if (it != m_cache.end()) {
            m_stats.cacheHits++;
            return it->second.get();
        }
    }

    // Cache miss - create new PSO (write lock)
    {
        ScopeLock lock(&m_cacheLock);

        // Double-check (another thread might have created it)
        auto it = m_cache.find(hash);
        if (it != m_cache.end()) {
            m_stats.cacheHits++;
            return it->second.get();
        }

        // Create PSO
        PipelineState* pso = CreatePipelineState(desc);

        if (pso) {
            m_cache[hash] = xr_unique_ptr<PipelineState>(pso);
            m_stats.psoCount++;
            m_stats.cacheMisses++;
            return pso;
        }

        return nullptr;
    }
}

PipelineState* PipelineStateCache::CreatePipelineState(
    const PipelineStateDesc& desc) {

    // Convert X-Ray desc -> NVRHI desc
    nvrhi::GraphicsPipelineDesc nvrhiDesc;

    // ─── Shaders ───
    if (desc.vertexShader) {
        nvrhiDesc.VS = desc.vertexShader;
        if (!nvrhiDesc.VS) {
            Msg("! [PipelineStateCache] Vertex shader has null native shader!");
        }
    } else {
        Msg("! [PipelineStateCache] No vertex shader provided!");
    }

    if (desc.pixelShader) {
        nvrhiDesc.PS = desc.pixelShader;
        if (!nvrhiDesc.PS) {
            Msg("! [PipelineStateCache] Pixel shader has null native shader!");
        }
    } else {
        Msg("! [PipelineStateCache] No pixel shader provided!");
    }

    if (desc.geometryShader)
        nvrhiDesc.GS = desc.geometryShader;
    if (desc.hullShader)
        nvrhiDesc.HS = desc.hullShader;
    if (desc.domainShader)
        nvrhiDesc.DS = desc.domainShader;

    // ─── Vertex Input Layout ───
    if (!desc.vertexAttributes.empty()) {
        xr_vector<nvrhi::VertexAttributeDesc> nvrhiAttrs;

        // NVRHI uses arraySize to handle multiple semantic indices!
        // If we have TEXCOORD0 and TEXCOORD1, we pass name="TEXCOORD" with arraySize=2
        // NVRHI will create D3D11 elements with SemanticIndex 0 and 1

        // Build NVRHI attributes preserving input order (critical for Vulkan location assignment).
        // Merge consecutive same-semantic attributes into one with arraySize > 1
        // (e.g., TEXCOORD0 + TEXCOORD1 → name="TEXCOORD", arraySize=2)
        for (size_t i = 0; i < desc.vertexAttributes.size(); ) {
            const auto& first = desc.vertexAttributes[i];
            if (!first.semanticName) { ++i; continue; }

            u32 count = 1;
            while (i + count < desc.vertexAttributes.size()) {
                const auto& next = desc.vertexAttributes[i + count];
                if (!next.semanticName)
                    break;
                if (xr_strcmp(next.semanticName, first.semanticName) != 0)
                    break;
                if (next.format != first.format)
                    break;
                if (next.bufferIndex != first.bufferIndex)
                    break;
                if (next.isInstanced != first.isInstanced)
                    break;
                ++count;
            }

            nvrhi::VertexAttributeDesc nvrhiAttr;
            nvrhiAttr.name = first.semanticName;
            nvrhiAttr.format = first.format;
            nvrhiAttr.offset = first.offset;
            nvrhiAttr.bufferIndex = first.bufferIndex;
            nvrhiAttr.isInstanced = first.isInstanced;
            nvrhiAttr.elementStride = first.elementStride;
            nvrhiAttr.arraySize = count;
            nvrhiAttrs.push_back(nvrhiAttr);
            i += count;
        }

        nvrhiDesc.inputLayout = m_device->GetNativeDevice()->createInputLayout(
            nvrhiAttrs.data(), (uint32_t)nvrhiAttrs.size(), desc.vertexShader);
    }

    // ─── Rasterizer State ───
    auto& rs = nvrhiDesc.renderState.rasterState;
    rs.cullMode = ConvertCullMode(desc.rasterizerState.cullMode);
    rs.fillMode = ConvertFillMode(desc.rasterizerState.fillMode);
    rs.frontCounterClockwise = desc.rasterizerState.frontCounterClockwise;
    rs.depthClipEnable = desc.rasterizerState.depthClipEnable;
    rs.scissorEnable = desc.rasterizerState.scissorEnable;
    rs.multisampleEnable = desc.rasterizerState.multisampleEnable;
    rs.antialiasedLineEnable = desc.rasterizerState.antialiasedLineEnable;
    rs.depthBias = desc.rasterizerState.depthBias;
    rs.depthBiasClamp = desc.rasterizerState.depthBiasClamp;
    rs.slopeScaledDepthBias = desc.rasterizerState.slopeScaledDepthBias;
    rs.conservativeRasterEnable = desc.rasterizerState.conservativeRaster;

    // ─── Blend State ───
    auto& bs = nvrhiDesc.renderState.blendState;
    bs.alphaToCoverageEnable = desc.blendState.alphaToCoverageEnable;

    // Initialize ALL 8 render targets to avoid uninitialized memory
    // NVRHI will use all 8 RTs when creating the D3D11 blend state
    for (u32 i = 0; i < 8; i++) {
        const auto& srcRT = desc.blendState.renderTargets[i];
        auto& dstRT = bs.targets[i];

        dstRT.blendEnable = srcRT.blendEnable;
        dstRT.srcBlend = ConvertBlendFactor(srcRT.srcBlend);
        dstRT.destBlend = ConvertBlendFactor(srcRT.dstBlend);
        dstRT.blendOp = ConvertBlendOp(srcRT.blendOp);
        dstRT.srcBlendAlpha = ConvertBlendFactor(srcRT.srcBlendAlpha);
        dstRT.destBlendAlpha = ConvertBlendFactor(srcRT.dstBlendAlpha);
        dstRT.blendOpAlpha = ConvertBlendOp(srcRT.blendOpAlpha);
        dstRT.colorWriteMask = static_cast<nvrhi::ColorMask>((u8)srcRT.writeMask);
    }

    // ─── Depth/Stencil State ───
    auto& ds = nvrhiDesc.renderState.depthStencilState;
    ds.depthTestEnable = desc.depthStencilState.depthTestEnable;
    ds.depthWriteEnable = desc.depthStencilState.depthWriteEnable;
    ds.depthFunc = ConvertComparisonFunc(desc.depthStencilState.depthFunc);
    ds.stencilEnable = desc.depthStencilState.stencilEnable;
    ds.stencilReadMask = desc.depthStencilState.stencilReadMask;
    ds.stencilWriteMask = desc.depthStencilState.stencilWriteMask;

    // Front face stencil
    ds.frontFaceStencil.failOp = ConvertStencilOp(desc.depthStencilState.frontFace.failOp);
    ds.frontFaceStencil.depthFailOp = ConvertStencilOp(desc.depthStencilState.frontFace.depthFailOp);
    ds.frontFaceStencil.passOp = ConvertStencilOp(desc.depthStencilState.frontFace.passOp);
    ds.frontFaceStencil.stencilFunc = ConvertComparisonFunc(desc.depthStencilState.frontFace.compareFunc);

    // Back face stencil
    ds.backFaceStencil.failOp = ConvertStencilOp(desc.depthStencilState.backFace.failOp);
    ds.backFaceStencil.depthFailOp = ConvertStencilOp(desc.depthStencilState.backFace.depthFailOp);
    ds.backFaceStencil.passOp = ConvertStencilOp(desc.depthStencilState.backFace.passOp);
    ds.backFaceStencil.stencilFunc = ConvertComparisonFunc(desc.depthStencilState.backFace.compareFunc);

    // ─── Primitive Topology ───
    nvrhiDesc.primType = ConvertPrimitiveTopology(desc.primitiveTopology);

    // ─── Binding Layouts (Root Signature) ───
    if (!desc.bindingLayouts.empty()) {
        for (const auto& layout : desc.bindingLayouts) {
            nvrhiDesc.bindingLayouts.push_back(layout);
        }
    }

    // ─── Create framebuffer info (needed for PSO creation) ───
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.width = 1920;   // Dummy values (not used for PSO validation)
    fbInfo.height = 1080;
    fbInfo.sampleCount = desc.sampleCount;
    fbInfo.sampleQuality = desc.sampleQuality;

    for (u32 i = 0; i < desc.renderTargetCount; i++) {
        fbInfo.colorFormats.push_back(desc.renderTargetFormats[i]);
    }
    fbInfo.depthFormat = desc.depthStencilFormat;

    // ─── Create NVRHI pipeline ───
    // Verbose PSO creation logging removed for performance
    // Msg("  [PipelineStateCache] Creating PSO: %s", desc.debugName.c_str());
    // Msg("    - VS: %s", nvrhiDesc.VS ? "valid" : "NULL");
    // Msg("    - PS: %s", nvrhiDesc.PS ? "valid" : "NULL");
    // Msg("    - Primitive type: %d", (int)nvrhiDesc.primType);
    // Msg("    - RT count: %d", fbInfo.colorFormats.size());

    // Ensure we have at least vertex shader (required)
    if (!nvrhiDesc.VS) {
        Msg("! [PipelineStateCache] Cannot create pipeline without vertex shader!");
        return nullptr;
    }

    nvrhi::GraphicsPipelineHandle nvrhiPipeline =
        m_device->GetNativeDevice()->createGraphicsPipeline(nvrhiDesc, fbInfo);

    if (!nvrhiPipeline) {
        Msg("! [PipelineStateCache] ❌ Failed to create PSO: %s",
            desc.debugName.c_str());
        Msg("! Check D3D11 debug layer for detailed error");
        return nullptr;
    }

    // ─── Wrap in X-Ray PSO ───
    return xr_new<PipelineState>(desc, nvrhiPipeline);
}

void PipelineStateCache::Clear() {
    ScopeLock lock(&m_cacheLock);
    m_cache.clear();
    m_stats.psoCount = 0;
    Msg("! [PipelineStateCache] Cache cleared");
}

void PipelineStateCache::ResetStats() {
    m_stats.cacheHits = 0;
    m_stats.cacheMisses = 0;
}

} // namespace xray::render::fg
