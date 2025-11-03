#pragma once

#include "xrCore/xrstring.h"
#include <d3d11shader.h>
#include <d3dcompiler.h>

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  RENDER PHASE TYPES
// ═══════════════════════════════════════════════════
//
// Determines which rendering phase a shader belongs to.
// Used for automatic pass routing and RT binding.
//
enum class RenderPhase {
    Geometry,      // Outputs to GBuffer (rt_Position, rt_Normal, rt_Color)
    Lighting,      // Outputs to rt_Accumulator
    Combine,       // Outputs to rt_Generic_0/1
    PostProcess,   // Various outputs (bloom, tonemap, etc.)
    Shadow,        // Shadow map generation
    Custom         // User-defined
};

// ═══════════════════════════════════════════════════
//  SHADER RT BINDINGS
// ═══════════════════════════════════════════════════
//
// Extracted from shader bytecode via D3D reflection.
// Contains all texture inputs/outputs for automatic RT binding.
//
struct ShaderRTBindings {
    RenderPhase phase = RenderPhase::Custom;

    // ─── Input Textures (SRVs) ───
    struct InputTexture {
        shared_str name;       // "s_position", "s_normal", etc.
        u32 slot;              // Texture slot (t0, t1, ...)

        InputTexture() : slot(0) {}
        InputTexture(const char* n, u32 s) : name(n), slot(s) {}
    };
    xr_vector<InputTexture> inputTextures;

    // ─── Samplers ───
    struct Sampler {
        shared_str name;       // "smp_base", "smp_rtlinear", etc.
        u32 slot;              // Sampler slot (s0, s1, ...)

        Sampler() : slot(0) {}
        Sampler(const char* n, u32 s) : name(n), slot(s) {}
    };
    xr_vector<Sampler> samplers;

    // ─── Output RTs (RTVs) ───
    enum class RTSemantic {
        Unknown,
        Normal,      // World-space or view-space normal
        Albedo,      // Base color / diffuse
        Material,    // Material properties (metallic, roughness, AO)
        Position,    // World-space or view-space position
        Emissive,    // Emissive color
        Accumulator  // Lighting accumulation buffer
    };

    struct OutputRT {
        u32 slot;              // SV_Target index
        RTSemantic semantic;   // Inferred semantic meaning
        shared_str formatDesc; // Format description for debugging

        OutputRT() : slot(0), semantic(RTSemantic::Unknown) {}
        OutputRT(u32 s) : slot(s), semantic(RTSemantic::Unknown) {}
    };
    xr_vector<OutputRT> outputRTs;

    // ─── Depth Output ───
    bool hasDepthOutput = false;

    // ─── Debug Info ───
    shared_str shaderName;
};

// ═══════════════════════════════════════════════════
//  SHADER REFLECTOR
// ═══════════════════════════════════════════════════
//
// Analyzes shader bytecode to extract render target bindings
// and infer rendering phase.
//
class ShaderReflector {
public:
    // Analyze pixel shader and extract RT bindings
    static ShaderRTBindings AnalyzePixelShader(
        ID3D11PixelShader* ps,
        ID3DBlob* bytecode);

    // Infer render phase from shader bindings
    static RenderPhase InferPhase(const ShaderRTBindings& bindings);

    // Get typical RT names for a phase
    static xr_vector<const char*> GetPhaseRTNames(RenderPhase phase);

    // Infer RT semantic from output signature
    static ShaderRTBindings::RTSemantic InferRTSemantic(
        u32 slot,
        u32 componentMask,
        RenderPhase phase);

private:
    // Helper: check if texture name matches pattern
    static bool MatchesPattern(const char* name, const char* pattern);
};

} // namespace xray::render::framegraph
