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
        return RenderPhase::Geometry;
    }

    Shader* shader = nullptr;
    ShaderElement* elem = nullptr;
    SPass* pass = nullptr;

    if (!GetShaderElements(visual, shader, elem, pass)) {
        return RenderPhase::Geometry;
    }

    ShaderKey key;
    if (!fg::ExtractShaderKey(visual, key)) {
        Msg("! [ShaderPhaseCache] Failed to extract shader key from visual");
        return RenderPhase::Geometry;
    }

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numHits++;
        return it->second.phase;
    }

    m_stats.numMisses++;
    RenderPhase phase = ExtractPhase(visual);

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
    Shader* shader = nullptr;
    ShaderElement* elem = nullptr;
    SPass* pass = nullptr;

    if (!GetShaderElements(visual, shader, elem, pass)) {
        return RenderPhase::Geometry;
    }

    ShaderKey key;
    fg::ExtractShaderKey(visual, key);
    std::string shaderName = key.ToString();

    auto* ps = pass->ps._get();
    if (!ps) {
        Msg("! [ShaderPhaseCache] No pixel shader for shader '%s'",
            shaderName.c_str());
        return RenderPhase::Geometry;
    }

    if (!ps->nvrhiShader) {
        Msg("! [ShaderPhaseCache] Failed to get NVRHI shader handle for shader '%s'",
            shaderName.c_str());
        return RenderPhase::Geometry;
    }

    if (!ps->reflection) {
        Msg("! [ShaderPhaseCache] No reflection for pixel shader '%s'",
            shaderName.c_str());
        return RenderPhase::Geometry;
    }

    const ShaderRTBindings& bindings = ShaderReflector::GetRTBindings(ps->reflection);

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

    outShader = visual->shader._get();
    if (!outShader) {
        return false;
    }

    outElement = outShader->E[0]._get();
    if (!outElement) {
        return false;
    }

    if (outElement->passes.empty()) {
        return false;
    }
    outPass = outElement->passes[0]._get();
    if (!outPass) {
        return false;
    }

    return true;
}

}
