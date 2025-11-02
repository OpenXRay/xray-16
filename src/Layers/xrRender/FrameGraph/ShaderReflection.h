#pragma once

#include "xrCore/xrstring.h"
#include <d3d11shader.h>
#include <d3dcompiler.h>

namespace xray::render::framegraph {

enum class RenderPhase {
    Geometry,
    Lighting,
    Combine,
    PostProcess,
    Shadow,
    Custom
};

struct ShaderRTBindings {
    RenderPhase phase = RenderPhase::Custom;

    struct InputTexture {
        shared_str name;
        u32 slot;

        InputTexture() : slot(0) {}
        InputTexture(const char* n, u32 s) : name(n), slot(s) {}
    };
    xr_vector<InputTexture> inputTextures;

    enum class RTSemantic {
        Unknown,
        Normal,
        Albedo,
        Material,
        Position,
        Emissive,
        Accumulator
    };

    struct OutputRT {
        u32 slot;
        RTSemantic semantic;
        shared_str formatDesc;

        OutputRT() : slot(0), semantic(RTSemantic::Unknown) {}
        OutputRT(u32 s) : slot(s), semantic(RTSemantic::Unknown) {}
    };
    xr_vector<OutputRT> outputRTs;

    bool hasDepthOutput = false;

    shared_str shaderName;
};

class ShaderReflector {
public:
    static ShaderRTBindings AnalyzePixelShader(
        ID3D11PixelShader* ps,
        ID3DBlob* bytecode);

    static RenderPhase InferPhase(const ShaderRTBindings& bindings);

    static xr_vector<const char*> GetPhaseRTNames(RenderPhase phase);

    static ShaderRTBindings::RTSemantic InferRTSemantic(
        u32 slot,
        u32 componentMask,
        RenderPhase phase);

private:
    static bool MatchesPattern(const char* name, const char* pattern);
};

}
