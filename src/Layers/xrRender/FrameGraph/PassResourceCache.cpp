#include "stdafx.h"
#include "PassResourceCache.h"
#include "BindingLayoutBuilder.h"
#include "ShaderCache.h"
#include "xrEngine/device.h"
#include "xrEngine/IGame_Persistent.h"
#include <cstring>

namespace xray::render::framegraph {

PassResourceCache& PassResourceCache::Instance() {
    static PassResourceCache instance;
    return instance;
}

PassResourceCache& GetPassResourceCache() {
    return PassResourceCache::Instance();
}

u64 PassResourceCache::HashString(const char* str) {
    u64 hash = 14695981039346656037ULL;
    while (*str) {
        hash ^= static_cast<u64>(*str++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

u64 PassResourceCache::HashCombine(u64 a, u64 b) {
    return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
}

u64 PassResourceCache::HashPointer(const void* ptr) {
    return static_cast<u64>(reinterpret_cast<uintptr_t>(ptr));
}

namespace {

u64 HashValue(u64 hash, u64 value) {
    return PassResourceCache::HashCombine(hash, value);
}

u64 HashFloat(u64 hash, float value) {
    u32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return HashValue(hash, bits);
}

u64 HashPtr(u64 hash, const void* ptr) {
    return HashValue(hash, PassResourceCache::HashPointer(ptr));
}

u64 HashSamplerDesc(const nvrhi::SamplerDesc& d) {
    u64 h = 0;
    h = HashFloat(h, d.borderColor.r);
    h = HashFloat(h, d.borderColor.g);
    h = HashFloat(h, d.borderColor.b);
    h = HashFloat(h, d.borderColor.a);
    h = HashFloat(h, d.maxAnisotropy);
    h = HashFloat(h, d.mipBias);
    h = HashValue(h, u64(d.minFilter) | u64(d.magFilter) << 1 | u64(d.mipFilter) << 2);
    h = HashValue(h, u64(d.addressU));
    h = HashValue(h, u64(d.addressV));
    h = HashValue(h, u64(d.addressW));
    return HashValue(h, u64(d.reductionType));
}

bool SamplerDescsEqual(const nvrhi::SamplerDesc& a, const nvrhi::SamplerDesc& b) {
    return a.borderColor == b.borderColor && a.maxAnisotropy == b.maxAnisotropy && a.mipBias == b.mipBias &&
        a.minFilter == b.minFilter && a.magFilter == b.magFilter && a.mipFilter == b.mipFilter &&
        a.addressU == b.addressU && a.addressV == b.addressV && a.addressW == b.addressW &&
        a.reductionType == b.reductionType;
}

u64 HashBindingLayoutDesc(const nvrhi::BindingLayoutDesc& d) {
    u64 h = HashValue(0, u64(d.visibility));
    h = HashValue(h, d.registerSpace);
    h = HashValue(h, u64(d.registerSpaceIsDescriptorSet));
    for (const auto& item : d.bindings) {
        h = HashValue(h, item.slot);
        h = HashValue(h, u64(item.type));
        h = HashValue(h, item.size);
    }
    h = HashValue(h, d.bindingOffsets.shaderResource);
    h = HashValue(h, d.bindingOffsets.sampler);
    h = HashValue(h, d.bindingOffsets.constantBuffer);
    return HashValue(h, d.bindingOffsets.unorderedAccess);
}

bool BindingLayoutDescsEqual(const nvrhi::BindingLayoutDesc& a, const nvrhi::BindingLayoutDesc& b) {
    return a.visibility == b.visibility && a.registerSpace == b.registerSpace &&
        a.registerSpaceIsDescriptorSet == b.registerSpaceIsDescriptorSet && a.bindings == b.bindings &&
        a.bindingOffsets.shaderResource == b.bindingOffsets.shaderResource &&
        a.bindingOffsets.sampler == b.bindingOffsets.sampler &&
        a.bindingOffsets.constantBuffer == b.bindingOffsets.constantBuffer &&
        a.bindingOffsets.unorderedAccess == b.bindingOffsets.unorderedAccess;
}

template <typename Map, typename Equal>
typename Map::mapped_type FindCached(Map& map, u64 key, Equal&& equal) {
    const auto range = map.equal_range(key);
    for (auto it = range.first; it != range.second; ++it)
        if (it->second && equal(it->second))
            return it->second;
    return nullptr;
}

}

nvrhi::SamplerHandle PassResourceCache::GetOrCreateSampler(
    const nvrhi::SamplerDesc& desc,
    nvrhi::IDevice* device)
{
    const u64 key = HashSamplerDesc(desc);
    if (nvrhi::SamplerHandle cached = FindCached(m_samplers, key,
            [&](const nvrhi::SamplerHandle& s) { return SamplerDescsEqual(s->getDesc(), desc); })) {
        m_stats.samplerHits++;
        return cached;
    }

    m_stats.samplerMisses++;
    nvrhi::SamplerHandle sampler = device->createSampler(desc);
    if (sampler) {
        m_samplers.emplace(key, sampler);
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
        desc.isRenderTarget = true;
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
        desc.isRenderTarget = true;
        desc.debugName = "DummyShadowMap2D";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.dimension = nvrhi::TextureDimension::Texture2D;
        m_dummyShadowMap2D = device->createTexture(desc);
    }
    return m_dummyShadowMap2D;
}

nvrhi::IBuffer* PassResourceCache::GetDummyBuffer(nvrhi::IDevice* device, bool uav) {
    nvrhi::BufferHandle& buffer = uav ? m_dummyUAVBuffer : m_dummySRVBuffer;
    if (!buffer) {
        nvrhi::BufferDesc desc;
        desc.byteSize = 256;
        desc.structStride = 16;
        desc.canHaveUAVs = uav;
        desc.canHaveRawViews = true;
        desc.debugName = uav ? "DummyUAVBuffer" : "DummySRVBuffer";
        desc.initialState = uav ? nvrhi::ResourceStates::UnorderedAccess : nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        buffer = device->createBuffer(desc);
        R_ASSERT(buffer);
    }
    return buffer;
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

nvrhi::BindingLayoutHandle PassResourceCache::GetOrCreateBindingLayout(
    const nvrhi::BindingLayoutDesc& desc,
    nvrhi::IDevice* device)
{
    const u64 key = HashBindingLayoutDesc(desc);
    if (nvrhi::BindingLayoutHandle cached = FindCached(m_bindingLayouts, key,
            [&](const nvrhi::BindingLayoutHandle& l) {
                const nvrhi::BindingLayoutDesc* existing = l->getDesc();
                return existing && BindingLayoutDescsEqual(*existing, desc);
            })) {
        m_stats.layoutHits++;
        return cached;
    }

    m_stats.layoutMisses++;
    nvrhi::BindingLayoutHandle layout = device->createBindingLayout(desc);
    if (layout) {
        m_bindingLayouts.emplace(key, layout);
    }
    return layout;
}

nvrhi::BindingLayoutHandle PassResourceCache::GetOrCreateBindingLayoutFromReflection(
    const ExtractedReflection& csReflection,
    nvrhi::IDevice* device)
{
    return GetOrCreateBindingLayout(BindingLayoutBuilder::Build(csReflection, nvrhi::ShaderType::Compute), device);
}

nvrhi::BindingLayoutHandle PassResourceCache::GetOrCreateBindingLayoutFromReflection(
    const ExtractedReflection& vsReflection,
    const ExtractedReflection& psReflection,
    nvrhi::IDevice* device)
{
    return GetOrCreateBindingLayout(BindingLayoutBuilder::Build(vsReflection, psReflection), device);
}

namespace {

u64 HashBindingLayouts(u64 h, const nvrhi::BindingLayoutVector& layouts) {
    h = HashValue(h, layouts.size());
    for (const auto& layout : layouts)
        h = HashPtr(h, layout.Get());
    return h;
}

bool BindingLayoutsEqual(const nvrhi::BindingLayoutVector& a, const nvrhi::BindingLayoutVector& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

u64 HashRasterState(u64 h, const nvrhi::RasterState& r) {
    h = HashValue(h, u64(r.fillMode));
    h = HashValue(h, u64(r.cullMode));
    h = HashValue(h, u64(r.frontCounterClockwise) | u64(r.depthClipEnable) << 1 | u64(r.scissorEnable) << 2 |
        u64(r.multisampleEnable) << 3 | u64(r.antialiasedLineEnable) << 4 |
        u64(r.programmableSamplePositionsEnable) << 5 | u64(r.conservativeRasterEnable) << 6 |
        u64(r.quadFillEnable) << 7);
    h = HashValue(h, u64(u32(r.depthBias)));
    h = HashFloat(h, r.depthBiasClamp);
    h = HashFloat(h, r.slopeScaledDepthBias);
    h = HashValue(h, r.forcedSampleCount);
    for (size_t i = 0; i < sizeof(r.samplePositionsX); ++i)
        h = HashValue(h, u64(u8(r.samplePositionsX[i])) | u64(u8(r.samplePositionsY[i])) << 8);
    return h;
}

bool RasterStatesEqual(const nvrhi::RasterState& a, const nvrhi::RasterState& b) {
    return a.fillMode == b.fillMode && a.cullMode == b.cullMode &&
        a.frontCounterClockwise == b.frontCounterClockwise && a.depthClipEnable == b.depthClipEnable &&
        a.scissorEnable == b.scissorEnable && a.multisampleEnable == b.multisampleEnable &&
        a.antialiasedLineEnable == b.antialiasedLineEnable && a.depthBias == b.depthBias &&
        a.depthBiasClamp == b.depthBiasClamp && a.slopeScaledDepthBias == b.slopeScaledDepthBias &&
        a.forcedSampleCount == b.forcedSampleCount &&
        a.programmableSamplePositionsEnable == b.programmableSamplePositionsEnable &&
        a.conservativeRasterEnable == b.conservativeRasterEnable && a.quadFillEnable == b.quadFillEnable &&
        std::memcmp(a.samplePositionsX, b.samplePositionsX, sizeof(a.samplePositionsX)) == 0 &&
        std::memcmp(a.samplePositionsY, b.samplePositionsY, sizeof(a.samplePositionsY)) == 0;
}

u64 HashStencilOp(u64 h, const nvrhi::DepthStencilState::StencilOpDesc& s) {
    return HashValue(h, u64(s.failOp) | u64(s.depthFailOp) << 8 | u64(s.passOp) << 16 | u64(s.stencilFunc) << 24);
}

bool StencilOpsEqual(const nvrhi::DepthStencilState::StencilOpDesc& a, const nvrhi::DepthStencilState::StencilOpDesc& b) {
    return a.failOp == b.failOp && a.depthFailOp == b.depthFailOp && a.passOp == b.passOp &&
        a.stencilFunc == b.stencilFunc;
}

u64 HashDepthStencilState(u64 h, const nvrhi::DepthStencilState& d) {
    h = HashValue(h, u64(d.depthTestEnable) | u64(d.depthWriteEnable) << 1 | u64(d.stencilEnable) << 2 |
        u64(d.dynamicStencilRef) << 3);
    h = HashValue(h, u64(d.depthFunc));
    h = HashValue(h, u64(d.stencilReadMask) | u64(d.stencilWriteMask) << 8 | u64(d.stencilRefValue) << 16);
    h = HashStencilOp(h, d.frontFaceStencil);
    return HashStencilOp(h, d.backFaceStencil);
}

bool DepthStencilStatesEqual(const nvrhi::DepthStencilState& a, const nvrhi::DepthStencilState& b) {
    return a.depthTestEnable == b.depthTestEnable && a.depthWriteEnable == b.depthWriteEnable &&
        a.depthFunc == b.depthFunc && a.stencilEnable == b.stencilEnable &&
        a.stencilReadMask == b.stencilReadMask && a.stencilWriteMask == b.stencilWriteMask &&
        a.stencilRefValue == b.stencilRefValue && a.dynamicStencilRef == b.dynamicStencilRef &&
        StencilOpsEqual(a.frontFaceStencil, b.frontFaceStencil) &&
        StencilOpsEqual(a.backFaceStencil, b.backFaceStencil);
}

u64 HashGraphicsPipeline(const nvrhi::GraphicsPipelineDesc& desc, const nvrhi::FramebufferInfo& fbInfo) {
    u64 h = HashValue(0, u64(desc.primType));
    h = HashValue(h, desc.patchControlPoints);
    h = HashPtr(h, desc.inputLayout.Get());
    h = HashPtr(h, desc.VS.Get());
    h = HashPtr(h, desc.HS.Get());
    h = HashPtr(h, desc.DS.Get());
    h = HashPtr(h, desc.GS.Get());
    h = HashPtr(h, desc.PS.Get());
    h = HashValue(h, std::hash<nvrhi::BlendState>()(desc.renderState.blendState));
    h = HashDepthStencilState(h, desc.renderState.depthStencilState);
    h = HashRasterState(h, desc.renderState.rasterState);
    const auto& sps = desc.renderState.singlePassStereo;
    h = HashValue(h, u64(sps.enabled) | u64(sps.independentViewportMask) << 1 | u64(sps.renderTargetIndexOffset) << 8);
    h = HashValue(h, std::hash<nvrhi::VariableRateShadingState>()(desc.shadingRateState));
    h = HashBindingLayouts(h, desc.bindingLayouts);
    return HashValue(h, std::hash<nvrhi::FramebufferInfo>()(fbInfo));
}

bool GraphicsPipelinesEqual(const nvrhi::IGraphicsPipeline& pipeline,
    const nvrhi::GraphicsPipelineDesc& desc, const nvrhi::FramebufferInfo& fbInfo)
{
    const nvrhi::GraphicsPipelineDesc& a = pipeline.getDesc();
    return a.primType == desc.primType && a.patchControlPoints == desc.patchControlPoints &&
        a.inputLayout == desc.inputLayout && a.VS == desc.VS && a.HS == desc.HS && a.DS == desc.DS &&
        a.GS == desc.GS && a.PS == desc.PS &&
        a.renderState.blendState == desc.renderState.blendState &&
        DepthStencilStatesEqual(a.renderState.depthStencilState, desc.renderState.depthStencilState) &&
        RasterStatesEqual(a.renderState.rasterState, desc.renderState.rasterState) &&
        a.renderState.singlePassStereo == desc.renderState.singlePassStereo &&
        a.shadingRateState == desc.shadingRateState &&
        BindingLayoutsEqual(a.bindingLayouts, desc.bindingLayouts) &&
        pipeline.getFramebufferInfo() == fbInfo;
}

u64 HashComputePipeline(const nvrhi::ComputePipelineDesc& desc) {
    return HashBindingLayouts(HashPtr(0, desc.CS.Get()), desc.bindingLayouts);
}

bool ComputePipelinesEqual(const nvrhi::ComputePipelineDesc& a, const nvrhi::ComputePipelineDesc& b) {
    return a.CS == b.CS && BindingLayoutsEqual(a.bindingLayouts, b.bindingLayouts);
}

}

void PassResourceCache::ReportPipelineCreation(const char* kind, const char* name, float milliseconds)
{
    if (!g_pGameLevel || Device.dwPrecacheFrame != 0 || !g_pGamePersistent || g_pGamePersistent->IsLoadingScreenShown())
        return;
    Msg("* [PipelineWarm] %s pipeline '%s' created during gameplay in %.1f ms", kind, name ? name : "", milliseconds);
}

nvrhi::GraphicsPipelineHandle PassResourceCache::GetOrCreatePipeline(
    const char* debugName,
    const nvrhi::GraphicsPipelineDesc& desc,
    const nvrhi::FramebufferInfo& fbInfo,
    nvrhi::IDevice* device)
{
    const u64 key = HashGraphicsPipeline(desc, fbInfo);
    if (nvrhi::GraphicsPipelineHandle cached = FindCached(m_graphicsPipelines, key,
            [&](const nvrhi::GraphicsPipelineHandle& p) { return GraphicsPipelinesEqual(*p, desc, fbInfo); })) {
        m_stats.pipelineHits++;
        return cached;
    }

    m_stats.pipelineMisses++;
    CTimer timer;
    timer.Start();
    nvrhi::GraphicsPipelineHandle pipeline = device->createGraphicsPipeline(desc, fbInfo);
    ReportPipelineCreation("graphics", debugName, timer.GetElapsed_sec() * 1000.f);
    if (pipeline) {
        m_graphicsPipelines.emplace(key, pipeline);
    }
    return pipeline;
}

nvrhi::ComputePipelineHandle PassResourceCache::GetOrCreateComputePipeline(
    const char* debugName,
    const nvrhi::ComputePipelineDesc& desc,
    nvrhi::IDevice* device)
{
    const u64 key = HashComputePipeline(desc);
    if (nvrhi::ComputePipelineHandle cached = FindCached(m_computePipelines, key,
            [&](const nvrhi::ComputePipelineHandle& p) { return ComputePipelinesEqual(p->getDesc(), desc); })) {
        m_stats.computePipelineHits++;
        return cached;
    }

    m_stats.computePipelineMisses++;
    CTimer timer;
    timer.Start();
    nvrhi::ComputePipelineHandle pipeline = device->createComputePipeline(desc);
    ReportPipelineCreation("compute", debugName, timer.GetElapsed_sec() * 1000.f);
    if (pipeline) {
        m_computePipelines.emplace(key, pipeline);
    }
    return pipeline;
}

static u64 HashFramebufferAttachment(u64 hash, const nvrhi::FramebufferAttachment& attachment)
{
    hash = PassResourceCache::HashCombine(hash, PassResourceCache::HashPointer(attachment.texture));
    hash = PassResourceCache::HashCombine(hash, attachment.subresources.baseMipLevel);
    hash = PassResourceCache::HashCombine(hash, attachment.subresources.numMipLevels);
    hash = PassResourceCache::HashCombine(hash, attachment.subresources.baseArraySlice);
    hash = PassResourceCache::HashCombine(hash, attachment.subresources.numArraySlices);
    hash = PassResourceCache::HashCombine(hash, u64(attachment.format));
    return PassResourceCache::HashCombine(hash, u64(attachment.isReadOnly));
}

static bool FramebufferAttachmentsEqual(
    const nvrhi::FramebufferAttachment& a, const nvrhi::FramebufferAttachment& b)
{
    return a.texture == b.texture &&
        a.subresources.baseMipLevel == b.subresources.baseMipLevel &&
        a.subresources.numMipLevels == b.subresources.numMipLevels &&
        a.subresources.baseArraySlice == b.subresources.baseArraySlice &&
        a.subresources.numArraySlices == b.subresources.numArraySlices &&
        a.format == b.format && a.isReadOnly == b.isReadOnly;
}

static bool FramebufferDescsEqual(const nvrhi::FramebufferDesc& a, const nvrhi::FramebufferDesc& b)
{
    if (a.colorAttachments.size() != b.colorAttachments.size() ||
        !FramebufferAttachmentsEqual(a.depthAttachment, b.depthAttachment) ||
        !FramebufferAttachmentsEqual(a.shadingRateAttachment, b.shadingRateAttachment))
        return false;
    for (size_t i = 0; i < a.colorAttachments.size(); ++i)
        if (!FramebufferAttachmentsEqual(a.colorAttachments[i], b.colorAttachments[i]))
            return false;
    return true;
}

nvrhi::FramebufferHandle PassResourceCache::GetOrCreateFramebuffer(
    const nvrhi::FramebufferDesc& desc,
    nvrhi::IDevice* device)
{
    u64 key = desc.colorAttachments.size();
    for (const auto& attachment : desc.colorAttachments)
        key = HashFramebufferAttachment(key, attachment);
    key = HashFramebufferAttachment(key, desc.depthAttachment);
    key = HashFramebufferAttachment(key, desc.shadingRateAttachment);

    const auto range = m_framebuffers.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
        if (FramebufferDescsEqual(desc, it->second->getDesc())) {
            m_stats.framebufferHits++;
            return it->second;
        }
    }

    m_stats.framebufferMisses++;
    nvrhi::FramebufferHandle fb = device->createFramebuffer(desc);
    if (fb) {
        m_framebuffers.emplace(key, fb);
    }
    return fb;
}

nvrhi::InputLayoutHandle PassResourceCache::GetOrCreateInputLayout(
    const nvrhi::VertexAttributeDesc* attrs,
    u32 attrCount,
    nvrhi::IShader* vertexShader,
    nvrhi::IDevice* device)
{
    u64 key = HashPtr(attrCount, vertexShader);
    for (u32 i = 0; i < attrCount; ++i) {
        const auto& a = attrs[i];
        key = HashValue(key, HashString(a.name.c_str()));
        key = HashValue(key, u64(a.format));
        key = HashValue(key, a.arraySize);
        key = HashValue(key, a.bufferIndex);
        key = HashValue(key, a.offset);
        key = HashValue(key, a.elementStride);
        key = HashValue(key, u64(a.isInstanced));
    }

    const auto range = m_inputLayouts.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
        const InputLayoutEntry& entry = it->second;
        if (entry.vertexShader != vertexShader || entry.layout->getNumAttributes() != attrCount)
            continue;
        bool equal = true;
        for (u32 i = 0; i < attrCount && equal; ++i) {
            const nvrhi::VertexAttributeDesc& a = *entry.layout->getAttributeDesc(i);
            const nvrhi::VertexAttributeDesc& b = attrs[i];
            equal = a.name == b.name && a.format == b.format && a.arraySize == b.arraySize &&
                a.bufferIndex == b.bufferIndex && a.offset == b.offset &&
                a.elementStride == b.elementStride && a.isInstanced == b.isInstanced;
        }
        if (equal) {
            m_stats.inputLayoutHits++;
            return entry.layout;
        }
    }

    m_stats.inputLayoutMisses++;
    nvrhi::InputLayoutHandle layout = device->createInputLayout(attrs, attrCount, vertexShader);
    if (layout) {
        m_inputLayouts.emplace(key, InputLayoutEntry{ vertexShader, layout });
    }
    return layout;
}

nvrhi::BufferHandle PassResourceCache::GetOrCreateStaticBuffer(
    const char* passName,
    const char* bufferName,
    const nvrhi::BufferDesc& desc,
    nvrhi::IDevice* device)
{
    u64 key = HashCombine(HashString(passName), HashString(bufferName));

    auto it = m_staticBuffers.find(key);
    if (it != m_staticBuffers.end()) {
        m_stats.bufferHits++;
        return it->second;
    }

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

nvrhi::BindingSetHandle PassResourceCache::GetOrCreateBindingSet(
    const nvrhi::BindingSetDesc& desc,
    nvrhi::IBindingLayout* layout,
    nvrhi::IDevice* device)
{
    const u64 key = HashValue(HashPtr(u64(desc.trackLiveness), layout), std::hash<nvrhi::BindingSetDesc>()(desc));
    if (nvrhi::BindingSetHandle cached = FindCached(m_bindingSets, key,
            [&](const nvrhi::BindingSetHandle& s) {
                const nvrhi::BindingSetDesc* existing = s->getDesc();
                return s->getLayout() == layout && existing && existing->trackLiveness == desc.trackLiveness &&
                    *existing == desc;
            })) {
        m_stats.bindingSetHits++;
        return cached;
    }

    m_stats.bindingSetMisses++;
    nvrhi::BindingSetHandle bindingSet = device->createBindingSet(desc, layout);
    if (bindingSet)
        m_bindingSets.emplace(key, bindingSet);
    return bindingSet;
}

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
    m_dummySRVBuffer = nullptr;
    m_dummyUAVBuffer = nullptr;

    Msg("* [PassResourceCache] Cleared all caches");
}

void PassResourceCache::ClearBindingSets() {
    m_bindingSets.clear();
}

void PassResourceCache::ClearFramebufferDependent() {
    m_framebuffers.clear();
    m_graphicsPipelines.clear();
    m_bindingSets.clear();
}

static bool FramebufferReferences(const nvrhi::FramebufferDesc& desc, const nvrhi::ITexture* texture) {
    for (const auto& attachment : desc.colorAttachments)
        if (attachment.texture == texture)
            return true;
    return desc.depthAttachment.texture == texture ||
        desc.shadingRateAttachment.texture == texture;
}

void PassResourceCache::InvalidateBindingSetsReferencing(const nvrhi::IResource* resource) {
    for (auto it = m_bindingSets.begin(); it != m_bindingSets.end();) {
        const nvrhi::BindingSetDesc* desc = it->second ? it->second->getDesc() : nullptr;
        bool references = false;
        if (desc) {
            for (const auto& binding : desc->bindings) {
                if (binding.resourceHandle == resource) {
                    references = true;
                    break;
                }
            }
        }
        if (references)
            it = m_bindingSets.erase(it);
        else
            ++it;
    }
}

void PassResourceCache::InvalidateTexture(const nvrhi::ITexture* texture) {
    if (!texture)
        return;

    for (auto it = m_framebuffers.begin(); it != m_framebuffers.end();) {
        if (it->second && FramebufferReferences(it->second->getDesc(), texture))
            it = m_framebuffers.erase(it);
        else
            ++it;
    }

    InvalidateBindingSetsReferencing(texture);
}

void PassResourceCache::InvalidateBuffer(const nvrhi::IBuffer* buffer) {
    if (buffer)
        InvalidateBindingSetsReferencing(buffer);
}

void PassResourceCache::ResetStats() {
    m_stats = Stats{};
}

}
