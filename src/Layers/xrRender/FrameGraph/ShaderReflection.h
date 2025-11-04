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
//  VERTEX SHADER INPUT SIGNATURE
// ═══════════════════════════════════════════════════
//
// Extracted from VS bytecode via D3D reflection.
// Defines the required vertex attributes in shader-expected order.
//
struct VertexInputSignature {
    struct InputElement {
        shared_str semanticName;   // "POSITION", "TEXCOORD", etc.
        u32 semanticIndex;          // 0, 1, 2, etc.
        DXGI_FORMAT format;         // R32G32B32_FLOAT, R8G8B8A8_UNORM, etc.
        u32 inputSlot;              // Buffer slot (usually 0)

        InputElement() : semanticIndex(0), format(DXGI_FORMAT_UNKNOWN), inputSlot(0) {}
    };
    xr_vector<InputElement> elements;  // In shader-expected order!
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
//  CONSTANT BUFFER INFO
// ═══════════════════════════════════════════════════
//
// Information about a constant buffer used by a shader.
// Used to determine required VCB sizes for different object types.
//
struct ConstantBufferInfo {
    shared_str name;      // "PerObject", "PerFrame", etc.
    u32 slot;             // Register slot (b0, b1, ...)
    u32 size;             // Size in bytes

    ConstantBufferInfo() : slot(0), size(0) {}
    ConstantBufferInfo(const char* n, u32 s, u32 sz) : name(n), slot(s), size(sz) {}
};

// ═══════════════════════════════════════════════════
//  SHADER CONSTANT BUFFER LAYOUT
// ═══════════════════════════════════════════════════
//
// Collection of all constant buffers used by a shader.
// Can be used to identify unique CB layouts and create appropriate VCBs.
//
struct ShaderConstantBuffers {
    xr_vector<ConstantBufferInfo> buffers;

    // Helper: Get CB by slot
    const ConstantBufferInfo* GetBySlot(u32 slot) const {
        for (const auto& cb : buffers) {
            if (cb.slot == slot) return &cb;
        }
        return nullptr;
    }

    // Helper: Get CB by name
    const ConstantBufferInfo* GetByName(const char* name) const {
        for (const auto& cb : buffers) {
            if (xr_strcmp(cb.name.c_str(), name) == 0) return &cb;
        }
        return nullptr;
    }
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
    // Analyze vertex shader and extract input signature (in shader-expected order!)
    static VertexInputSignature AnalyzeVertexShader(
        ID3D11VertexShader* vs,
        ID3DBlob* bytecode);

    // Analyze pixel shader and extract RT bindings
    static ShaderRTBindings AnalyzePixelShader(
        ID3D11PixelShader* ps,
        ID3DBlob* bytecode);

    // Analyze shader constant buffers (works for VS/PS/CS)
    static ShaderConstantBuffers AnalyzeConstantBuffers(ID3DBlob* bytecode);

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
