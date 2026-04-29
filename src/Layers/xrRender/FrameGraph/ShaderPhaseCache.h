#pragma once

#include "ShaderReflection.h"
#include "Layers/xrRender/ShaderKey.h"

namespace xray::render::fg {
    struct Shader;
    struct ShaderElement;
    struct SPass;
    class dxRender_Visual;
}

namespace xray::render::framegraph {

using fg::Shader;
using fg::ShaderElement;
using fg::SPass;
using fg::dxRender_Visual;
using fg::ShaderKey;

class ShaderPhaseCache {
public:
    ShaderPhaseCache();
    ~ShaderPhaseCache();

    RenderPhase GetPhase(dxRender_Visual* visual);

    void Clear();

    struct Stats {
        u32 numCached = 0;
        u32 numHits = 0;
        u32 numMisses = 0;
    };

    const Stats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats.numHits = 0; m_stats.numMisses = 0; }

private:
    struct CacheEntry {
        RenderPhase phase;
    };

    xr_map<ShaderKey, CacheEntry> m_cache;
    Stats m_stats;

    RenderPhase ExtractPhase(dxRender_Visual* visual);

    bool GetShaderElements(dxRender_Visual* visual,
                          Shader*& outShader,
                          ShaderElement*& outElement,
                          SPass*& outPass);
};

}
