// PassResourceCache.cpp
// Implementation of centralized pass resource caching
#include "stdafx.h"
#include "PassResourceCache.h"

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

// ═══════════════════════════════════════════════════════════════════════════════
//  GRAPHICS PIPELINE CACHE
// ═══════════════════════════════════════════════════════════════════════════════

nvrhi::GraphicsPipelineHandle PassResourceCache::GetOrCreatePipeline(
    const char* passName,
    const nvrhi::GraphicsPipelineDesc& desc,
    const nvrhi::FramebufferInfo& fbInfo,
    nvrhi::IDevice* device)
{
    // Key is just pass name - each pass has one pipeline configuration
    u64 key = HashString(passName);

    auto it = m_graphicsPipelines.find(key);
    if (it != m_graphicsPipelines.end()) {
        m_stats.pipelineHits++;
        return it->second;
    }

    // Create new pipeline using FramebufferInfo (preferred API)
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

    Msg("* [PassResourceCache] Cleared all caches");
}

void PassResourceCache::ResetStats() {
    m_stats = Stats{};
}

} // namespace xray::render::framegraph
