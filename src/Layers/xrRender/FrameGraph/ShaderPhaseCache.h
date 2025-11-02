#pragma once

#include "ShaderReflection.h"
#include "xrCore/xrstring.h"

namespace xray::render::RENDER_NAMESPACE {
    struct Shader;
    struct ShaderElement;
    struct SPass;
    class dxRender_Visual;
}

namespace xray::render::framegraph {

using RENDER_NAMESPACE::Shader;
using RENDER_NAMESPACE::ShaderElement;
using RENDER_NAMESPACE::SPass;
using RENDER_NAMESPACE::dxRender_Visual;

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
    struct CacheKey {
        shared_str shaderName;

        bool operator<(const CacheKey& other) const {
            return shaderName < other.shaderName;
        }
    };

    struct CacheEntry {
        RenderPhase phase;
    };

    xr_map<CacheKey, CacheEntry> m_cache;
    Stats m_stats;

    RenderPhase ExtractPhase(dxRender_Visual* visual);

    bool GetShaderElements(dxRender_Visual* visual,
                          Shader*& outShader,
                          ShaderElement*& outElement,
                          SPass*& outPass);
};

}
