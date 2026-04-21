// PassResourceCache.cpp
// Implementation of centralized pass resource caching
#include "stdafx.h"
#include "PassResourceCache.h"
#include "BindingLayoutBuilder.h"
#include "ShaderCache.h"

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════════════════════════════════
//  SINGLETON INSTANCE
// ═══════════════════════════════════════════════════════════════════════════════

PassResourceCache& PassResourceCache::Instance() {
    static PassResourceCache instance;
    return instance;
}

PassResourceCache& GetPassResourceCache() {
    return PassResourceCache::Instance();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  HASH HELPERS
// ═══════════════════════════════════════════════════════════════════════════════

u64 PassResourceCache::HashString(const char* str) {
    // FNV-1a hash
    u64 hash = 14695981039346656037ULL;
    while (*str) {
        hash ^= static_cast<u64>(*str++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

u64 PassResourceCache::HashCombine(u64 a, u64 b) {
    // Boost-style hash combine
    return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
}

u64 PassResourceCache::HashPointer(const void* ptr) {
    return static_cast<u64>(reinterpret_cast<uintptr_t>(ptr));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SAMPLER CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::SamplerHandle PassResourceCache::GetOrCreateSampler(
    const char* passName,
    const nvrhi::SamplerDesc& desc,
    nvrhi::IDevice* device)
{
    u64 key = HashString(passName);

    auto it = m_samplers.find(key);
    if (it != m_samplers.end()) {
        m_stats.samplerHits++;
        return it->second;
    }

    // Create new sampler
    m_stats.samplerMisses++;
    nvrhi::SamplerHandle sampler = device->createSampler(desc);
    if (sampler) {
        m_samplers[key] = sampler;
    }
    return sampler;
}

nvrhi::ISampler* PassResourceCache::GetAnisoWrapSampler(nvrhi::IDevice* device) {
    if (!m_commonAnisoWrap) {
        nvrhi::SamplerDesc desc;
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        desc.setAllFilters(true);
        desc.setMaxAnisotropy(16.0f);
        m_commonAnisoWrap = device->createSampler(desc);
    }
    return m_commonAnisoWrap;
}

nvrhi::ISampler* PassResourceCache::GetLinearWrapSampler(nvrhi::IDevice* device) {
    if (!m_commonLinearWrap) {
        nvrhi::SamplerDesc desc;
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        desc.setAllFilters(true);
        m_commonLinearWrap = device->createSampler(desc);
    }
    return m_commonLinearWrap;
}

nvrhi::ISampler* PassResourceCache::GetLinearClampSampler(nvrhi::IDevice* device) {
    if (!m_commonLinearClamp) {
        nvrhi::SamplerDesc desc;
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge);
        desc.setAllFilters(true);
        m_commonLinearClamp = device->createSampler(desc);
    }
    return m_commonLinearClamp;
}

nvrhi::ISampler* PassResourceCache::GetPointClampSampler(nvrhi::IDevice* device) {
    if (!m_commonPointClamp) {
        nvrhi::SamplerDesc desc;
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge);
        desc.setAllFilters(false);
        m_commonPointClamp = device->createSampler(desc);
    }
    return m_commonPointClamp;
}

nvrhi::ISampler* PassResourceCache::GetShadowCmpSampler(nvrhi::IDevice* device) {
    if (!m_commonShadowCmp) {
        nvrhi::SamplerDesc desc;
        desc.reductionType = nvrhi::SamplerReductionType::Comparison;
        desc.minFilter = true;
        desc.magFilter = true;
        desc.mipFilter = false;
        desc.addressU = nvrhi::SamplerAddressMode::Border;
        desc.addressV = nvrhi::SamplerAddressMode::Border;
        desc.addressW = nvrhi::SamplerAddressMode::Border;
        desc.borderColor = nvrhi::Color(1.0f);
        m_commonShadowCmp = device->createSampler(desc);
    }
    return m_commonShadowCmp;
}

nvrhi::ITexture* PassResourceCache::GetDummyShadowMap(nvrhi::IDevice* device) {
    if (!m_dummyShadowMap) {
        nvrhi::TextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.arraySize = 3;
        desc.format = nvrhi::Format::D32;
        desc.debugName = "DummyShadowMap";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.dimension = nvrhi::TextureDimension::Texture2DArray;
        m_dummyShadowMap = device->createTexture(desc);
    }
    return m_dummyShadowMap;
}

nvrhi::ITexture* PassResourceCache::GetDummyShadowMap2D(nvrhi::IDevice* device) {
    if (!m_dummyShadowMap2D) {
        nvrhi::TextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.format = nvrhi::Format::D32;
        desc.debugName = "DummyShadowMap2D";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.dimension = nvrhi::TextureDimension::Texture2D;
        m_dummyShadowMap2D = device->createTexture(desc);
    }
    return m_dummyShadowMap2D;
}

nvrhi::ISampler* PassResourceCache::GetSamplerByName(const char* smpName, nvrhi::IDevice* device)
{
    if (strstr(smpName, "smp_nofilter") || strstr(smpName, "smp_smap") || strstr(smpName, "smp_jitter"))
        return GetPointClampSampler(device);
    if (strstr(smpName, "smp_rtlinear"))
        return GetLinearClampSampler(device);
    if (strstr(smpName, "smp_base") || strstr(smpName, "smp_material") || strstr(smpName, "smp_bump"))
        return GetAnisoWrapSampler(device);
    if (strstr(smpName, "smp_shadowcmp"))
        return GetShadowCmpSampler(device);
    return GetLinearWrapSampler(device);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  BINDING LAYOUT CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::BindingLayoutHandle PassResourceCache::GetOrCreateBindingLayout(
    const char* passName,
    const nvrhi::BindingLayoutDesc& desc,
    nvrhi::IDevice* device)
{
    u64 key = HashString(passName);

    auto it = m_bindingLayouts.find(key);
    if (it != m_bindingLayouts.end()) {
        m_stats.layoutHits++;
        return it->second;
    }

    // Create new layout
    m_stats.layoutMisses++;
    nvrhi::BindingLayoutHandle layout = device->createBindingLayout(desc);
    if (layout) {
        m_bindingLayouts[key] = layout;
    }
    return layout;
}

nvrhi::BindingLayoutHandle PassResourceCache::GetOrCreateBindingLayoutFromReflection(
    const char* passName,
    const ExtractedReflection& csReflection,
    nvrhi::IDevice* device)
{
    auto desc = BindingLayoutBuilder::Build(csReflection, nvrhi::ShaderType::Compute);
    return GetOrCreateBindingLayout(passName, desc, device);
}

nvrhi::BindingLayoutHandle PassResourceCache::GetOrCreateBindingLayoutFromReflection(
    const char* passName,
    const ExtractedReflection& vsReflection,
    const ExtractedReflection& psReflection,
    nvrhi::IDevice* device)
{
    auto desc = BindingLayoutBuilder::Build(vsReflection, psReflection);
    return GetOrCreateBindingLayout(passName, desc, device);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  GRAPHICS PIPELINE CACHE
// ═══════════════════════════════════════════════════════════════════════════════

static u64 HashGraphicsPipelineKey(const char* passName,
    const nvrhi::GraphicsPipelineDesc& desc,
    const nvrhi::FramebufferInfo& fbInfo)
{
    u64 hash = PassResourceCache::HashString(passName);
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(desc.VS.Get()));
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(desc.PS.Get()));
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(desc.GS.Get()));
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(desc.HS.Get()));
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(desc.DS.Get()));
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(desc.inputLayout.Get()));
    hash = PassResourceCache::HashCombine(hash, u64(desc.primType));
    for (const auto& layout : desc.bindingLayouts)
        hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(layout.Get()));
    for (const auto& fmt : fbInfo.colorFormats)
        hash = PassResourceCache::HashCombine(hash, u64(fmt));
    hash = PassResourceCache::HashCombine(hash, u64(fbInfo.depthFormat));
    hash = PassResourceCache::HashCombine(hash, u64(fbInfo.sampleCount));
    const auto& rs = desc.renderState;
    hash = PassResourceCache::HashCombine(hash, u64(rs.depthStencilState.depthTestEnable));
    hash = PassResourceCache::HashCombine(hash, u64(rs.depthStencilState.depthWriteEnable));
    hash = PassResourceCache::HashCombine(hash, u64(rs.depthStencilState.depthFunc));
    hash = PassResourceCache::HashCombine(hash, u64(rs.rasterState.cullMode));
    hash = PassResourceCache::HashCombine(hash, u64(rs.rasterState.frontCounterClockwise));
    const auto& bt = rs.blendState.targets[0];
    hash = PassResourceCache::HashCombine(hash, u64(bt.blendEnable));
    hash = PassResourceCache::HashCombine(hash, u64(bt.srcBlend));
    hash = PassResourceCache::HashCombine(hash, u64(bt.destBlend));
    hash = PassResourceCache::HashCombine(hash, u64(bt.blendOp));
    return hash;
}

nvrhi::GraphicsPipelineHandle PassResourceCache::GetOrCreatePipeline(
    const char* passName,
    const nvrhi::GraphicsPipelineDesc& desc,
    const nvrhi::FramebufferInfo& fbInfo,
    nvrhi::IDevice* device)
{
    u64 key = HashGraphicsPipelineKey(passName, desc, fbInfo);

    auto it = m_graphicsPipelines.find(key);
    if (it != m_graphicsPipelines.end()) {
        m_stats.pipelineHits++;
        return it->second;
    }

    m_stats.pipelineMisses++;
    nvrhi::GraphicsPipelineHandle pipeline = device->createGraphicsPipeline(desc, fbInfo);
    if (pipeline) {
        m_graphicsPipelines[key] = pipeline;
    }
    return pipeline;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  COMPUTE PIPELINE CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::ComputePipelineHandle PassResourceCache::GetOrCreateComputePipeline(
    const char* passName,
    const nvrhi::ComputePipelineDesc& desc,
    nvrhi::IDevice* device)
{
    u64 key = HashString(passName);

    auto it = m_computePipelines.find(key);
    if (it != m_computePipelines.end()) {
        m_stats.computePipelineHits++;
        return it->second;
    }

    // Create new compute pipeline
    m_stats.computePipelineMisses++;
    nvrhi::ComputePipelineHandle pipeline = device->createComputePipeline(desc);
    if (pipeline) {
        m_computePipelines[key] = pipeline;
    }
    return pipeline;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  FRAMEBUFFER CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::FramebufferHandle PassResourceCache::GetOrCreateFramebuffer(
    const char* passName,
    const nvrhi::FramebufferDesc& desc,
    nvrhi::IDevice* device)
{
    // Key combines pass name with all render target pointers
    // This ensures we reuse framebuffers when the same RTs are bound
    u64 key = HashString(passName);

    for (const auto& attachment : desc.colorAttachments) {
        if (attachment.texture) {
            key = HashCombine(key, HashPointer(attachment.texture));
        }
    }
    if (desc.depthAttachment.texture) {
        key = HashCombine(key, HashPointer(desc.depthAttachment.texture));
    }

    auto it = m_framebuffers.find(key);
    if (it != m_framebuffers.end()) {
        m_stats.framebufferHits++;
        return it->second;
    }

    // Create new framebuffer
    m_stats.framebufferMisses++;
    nvrhi::FramebufferHandle fb = device->createFramebuffer(desc);
    if (fb) {
        m_framebuffers[key] = fb;
    }
    return fb;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  INPUT LAYOUT CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::InputLayoutHandle PassResourceCache::GetOrCreateInputLayout(
    const char* passName,
    const nvrhi::VertexAttributeDesc* attrs,
    u32 attrCount,
    nvrhi::IShader* vertexShader,
    nvrhi::IDevice* device)
{
    // Key combines pass name with vertex shader pointer
    u64 key = HashCombine(HashString(passName), HashPointer(vertexShader));

    auto it = m_inputLayouts.find(key);
    if (it != m_inputLayouts.end()) {
        m_stats.inputLayoutHits++;
        return it->second;
    }

    // Create new input layout
    m_stats.inputLayoutMisses++;
    nvrhi::InputLayoutHandle layout = device->createInputLayout(attrs, attrCount, vertexShader);
    if (layout) {
        m_inputLayouts[key] = layout;
    }
    return layout;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  STATIC BUFFER CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::BufferHandle PassResourceCache::GetOrCreateStaticBuffer(
    const char* passName,
    const char* bufferName,
    const nvrhi::BufferDesc& desc,
    nvrhi::IDevice* device)
{
    // Key combines pass name with buffer name
    u64 key = HashCombine(HashString(passName), HashString(bufferName));

    auto it = m_staticBuffers.find(key);
    if (it != m_staticBuffers.end()) {
        m_stats.bufferHits++;
        return it->second;
    }

    // Create new buffer
    m_stats.bufferMisses++;
    nvrhi::BufferHandle buffer = device->createBuffer(desc);
    if (buffer) {
        m_staticBuffers[key] = buffer;
    }
    return buffer;
}

bool PassResourceCache::HasStaticBuffer(const char* passName, const char* bufferName) const {
    u64 key = HashCombine(HashString(passName), HashString(bufferName));
    return m_staticBuffers.find(key) != m_staticBuffers.end();
}

nvrhi::IBuffer* PassResourceCache::GetOrCreateVolatileCB(
    const char* passName, const char* bufferName,
    u32 byteSize, fg::RenderDevice* device, u32 maxVersions)
{
    u64 key = HashCombine(HashString(passName), HashString(bufferName));
    auto it = m_volatileCBs.find(key);
    if (it != m_volatileCBs.end()) {
        m_stats.bufferHits++;
        return device->GetNativeBuffer(it->second);
    }
    m_stats.bufferMisses++;

    fg::RenderDevice::BufferDesc desc;
    desc.byteSize = byteSize;
    desc.isConstantBuffer = true;
    desc.isVolatile = true;
    desc.maxVersions = maxVersions;

    string256 debugStr;
    xr_sprintf(debugStr, "VCB_%s_%s", passName, bufferName);
    desc.debugName = debugStr;

    fg::BufferHandle handle = device->CreateBuffer(desc);
    if (handle.IsValid()) {
        m_volatileCBs[key] = handle;
        Msg("* [VCB] Created '%s' (%u bytes, %u versions, total %u bytes)",
            debugStr, byteSize, maxVersions, byteSize * maxVersions);
        return device->GetNativeBuffer(handle);
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  BINDING SET CACHE
// ═══════════════════════════════════════════════════════════════════════════════

static u64 HashBindingSetDesc(const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout) {
    u64 hash = PassResourceCache::HashPointer(layout);
    for (const auto& item : desc.bindings) {
        hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(item.resourceHandle));
        hash = PassResourceCache::HashCombine(hash, u64(item.slot));
        hash = PassResourceCache::HashCombine(hash, u64(item.type));
        hash = PassResourceCache::HashCombine(hash, u64(item.format));
        hash = PassResourceCache::HashCombine(hash, u64(item.dimension));
        hash = PassResourceCache::HashCombine(hash, item.rawData[0]);
        hash = PassResourceCache::HashCombine(hash, item.rawData[1]);
    }
    return hash;
}

nvrhi::BindingSetHandle PassResourceCache::GetOrCreateBindingSet(
    const nvrhi::BindingSetDesc& desc,
    nvrhi::IBindingLayout* layout,
    nvrhi::IDevice* device)
{
    u64 key = HashBindingSetDesc(desc, layout);
    auto it = m_bindingSets.find(key);
    if (it != m_bindingSets.end()) {
        m_stats.bindingSetHits++;
        return it->second;
    }

    m_stats.bindingSetMisses++;
    nvrhi::BindingSetHandle bindingSet = device->createBindingSet(desc, layout);
    if (bindingSet)
        m_bindingSets[key] = bindingSet;
    return bindingSet;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════════

void PassResourceCache::Clear() {
    m_samplers.clear();
    m_bindingLayouts.clear();
    m_graphicsPipelines.clear();
    m_computePipelines.clear();
    m_framebuffers.clear();
    m_inputLayouts.clear();
    m_staticBuffers.clear();
    m_volatileCBs.clear();
    m_bindingSets.clear();
    m_commonAnisoWrap = nullptr;
    m_commonLinearWrap = nullptr;
    m_commonLinearClamp = nullptr;
    m_commonPointClamp = nullptr;
    m_commonShadowCmp = nullptr;
    m_dummyShadowMap = nullptr;
    m_dummyShadowMap2D = nullptr;

    Msg("* [PassResourceCache] Cleared all caches");
}

void PassResourceCache::ResetStats() {
    m_stats = Stats{};
}

} // namespace xray::render::framegraph
