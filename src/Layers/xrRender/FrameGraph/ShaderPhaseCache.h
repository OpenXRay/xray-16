// xrRender/FrameGraph/ShaderPhaseCache.h
#pragma once

#include "ShaderReflection.h"
#include "xrCore/xrstring.h"

// Forward declarations
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

// ═══════════════════════════════════════════════════
//  SHADER PHASE CACHE
// ═══════════════════════════════════════════════════
//
// Lightweight cache for shader phase information.
// Extracts phase from shader reflection WITHOUT creating full PSO.
// Used during phase scanning before FrameGraph compilation.
//
// Key difference from MaterialPSO:
// - MaterialPSO: Full PSO + bindings (requires physical RTs)
// - ShaderPhaseCache: Just phase info (no RT dependencies)
//
class ShaderPhaseCache {
public:
    ShaderPhaseCache();
    ~ShaderPhaseCache();

    // ═══════════════════════════════════════════════════
    //  PHASE QUERY
    // ═══════════════════════════════════════════════════

    // Get phase for a visual's shader (cached or extracted)
    RenderPhase GetPhase(dxRender_Visual* visual);

    // Clear cache
    void Clear();

    // Statistics
    struct Stats {
        u32 numCached = 0;
        u32 numHits = 0;
        u32 numMisses = 0;
    };

    const Stats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats.numHits = 0; m_stats.numMisses = 0; }

private:
    // Cache key: shader name
    struct CacheKey {
        shared_str shaderName;

        bool operator<(const CacheKey& other) const {
            return shaderName < other.shaderName;
        }
    };

    // Cache entry: just the phase
    struct CacheEntry {
        RenderPhase phase;
    };

    // Cache storage
    xr_map<CacheKey, CacheEntry> m_cache;
    Stats m_stats;

    // Extract phase from shader (runs reflection)
    RenderPhase ExtractPhase(dxRender_Visual* visual);

    // Helper: Get shader elements from visual
    bool GetShaderElements(dxRender_Visual* visual,
                          Shader*& outShader,
                          ShaderElement*& outElement,
                          SPass*& outPass);
};

} // namespace xray::render::framegraph
