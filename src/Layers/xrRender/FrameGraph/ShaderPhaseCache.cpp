// xrRender/FrameGraph/ShaderPhaseCache.cpp
#include "stdafx.h"
#include "ShaderPhaseCache.h"
#include "ShaderReflection.h"
#include "IPass.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/ShaderKey.h"

namespace xray::render::framegraph {

ShaderPhaseCache::ShaderPhaseCache() {
    Msg("* [ShaderPhaseCache] Created");
}

ShaderPhaseCache::~ShaderPhaseCache() {
    Msg("* [ShaderPhaseCache] Destroyed (cached %u shaders)", m_stats.numCached);
}

RenderPhase ShaderPhaseCache::GetPhase(dxRender_Visual* visual) {
    if (!visual) {
        return RenderPhase::Geometry;  // Default fallback
    }

    // Get shader info
    Shader* shader = nullptr;
    ShaderElement* elem = nullptr;
    SPass* pass = nullptr;

    if (!GetShaderElements(visual, shader, elem, pass)) {
        return RenderPhase::Geometry;  // Default fallback
    }

    // Create cache key using shader names (production-safe)
    ShaderKey key;
    if (!fg::ExtractShaderKey(visual, key)) {
        Msg("! [ShaderPhaseCache] Failed to extract shader key from visual");
        return RenderPhase::Geometry;
    }

    // Check cache
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numHits++;
        return it->second.phase;
    }

    // Cache miss - extract phase
    m_stats.numMisses++;
    RenderPhase phase = ExtractPhase(visual);

    // Store in cache
    CacheEntry entry;
    entry.phase = phase;
    m_cache[key] = entry;
    m_stats.numCached = static_cast<u32>(m_cache.size());

    return phase;
}

void ShaderPhaseCache::Clear() {
    m_cache.clear();
    m_stats = Stats{};
}

RenderPhase ShaderPhaseCache::ExtractPhase(dxRender_Visual* visual) {
    // Get shader info
    Shader* shader = nullptr;
    ShaderElement* elem = nullptr;
    SPass* pass = nullptr;

    if (!GetShaderElements(visual, shader, elem, pass)) {
        return RenderPhase::Geometry;
    }

    // ═══════════════════════════════════════════════════════
    //  RUN SHADER REFLECTION ON PIXEL SHADER
    // ═══════════════════════════════════════════════════════

    // Extract shader key for logging
    ShaderKey key;
    fg::ExtractShaderKey(visual, key);
    std::string shaderName = key.ToString();

    // Get pixel shader (ref_ps is a smart pointer, use _get())
    auto* ps = pass->ps._get();
    if (!ps) {
        Msg("! [ShaderPhaseCache] No pixel shader for shader '%s'",
            shaderName.c_str());
        return RenderPhase::Geometry;
    }

    // Check if shader has NVRHI handle
    if (!ps->nvrhiShader) {
        Msg("! [ShaderPhaseCache] Failed to get NVRHI shader handle for shader '%s'",
            shaderName.c_str());
        return RenderPhase::Geometry;
    }

    // Get extracted reflection (stored during shader creation)
    if (!ps->reflection) {
        Msg("! [ShaderPhaseCache] No reflection for pixel shader '%s'",
            shaderName.c_str());
        return RenderPhase::Geometry;
    }

    // Get RT bindings from extracted reflection
    const ShaderRTBindings& bindings = ShaderReflector::GetRTBindings(ps->reflection);

    // Log result
    Msg("! [ShaderPhaseCache] Extracted phase for shader '%s': %s",
        shaderName.c_str(),
        IPass::GetPhaseName(bindings.phase));

    return bindings.phase;
}

bool ShaderPhaseCache::GetShaderElements(
    dxRender_Visual* visual,
    Shader*& outShader,
    ShaderElement*& outElement,
    SPass*& outPass)
{
    if (!visual) {
        return false;
    }

    // Get shader from visual
    outShader = visual->shader._get();
    if (!outShader) {
        return false;
    }

    outElement = outShader->E[0]._get();
    if (!outElement) {
        return false;
    }

    // Get first pass
    if (outElement->passes.empty()) {
        return false;
    }
    outPass = outElement->passes[0]._get();
    if (!outPass) {
        return false;
    }

    return true;
}

} // namespace xray::render::framegraph
