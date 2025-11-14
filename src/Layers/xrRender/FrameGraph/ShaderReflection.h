#pragma once

#include "xrCore/xrstring.h"

namespace slang {
    struct ShaderReflection;
}

namespace xray::render::framegraph {

enum class RenderPhase {
    Geometry,
    Lighting,
    Combine,
    PostProcess,
    Shadow,
    Custom
};

struct VertexInputSignature {
    struct InputElement {
        shared_str semanticName;
        u32 semanticIndex;
        nvrhi::Format format;
        u32 inputSlot;

        InputElement() : semanticIndex(0), format(nvrhi::Format::UNKNOWN), inputSlot(0) {}
    };
    xr_vector<InputElement> elements;
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

    struct Sampler {
        shared_str name;
        u32 slot;

        Sampler() : slot(0) {}
        Sampler(const char* n, u32 s) : name(n), slot(s) {}
    };
    xr_vector<Sampler> samplers;

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

struct ConstantBufferInfo {
    shared_str name;
    u32 slot;
    u32 size;

    ConstantBufferInfo() : slot(0), size(0) {}
    ConstantBufferInfo(const char* n, u32 s, u32 sz) : name(n), slot(s), size(sz) {}
};

struct ShaderConstantBuffers {
    xr_vector<ConstantBufferInfo> buffers;

    const ConstantBufferInfo* GetBySlot(u32 slot) const {
        for (const auto& cb : buffers) {
            if (cb.slot == slot) return &cb;
        }
        return nullptr;
    }

    const ConstantBufferInfo* GetByName(const char* name) const {
        for (const auto& cb : buffers) {
            if (xr_strcmp(cb.name.c_str(), name) == 0) return &cb;
        }
        return nullptr;
    }
};

class ShaderReflector {
public:

    static ExtractedReflection ExtractReflection(
        slang::ShaderReflection* slangReflection,
        bool isVertexShader);

    static const VertexInputSignature& GetVertexInputSignature(
        const ExtractedReflection* reflection);

    static const ShaderRTBindings& GetRTBindings(
        const ExtractedReflection* reflection);

    static const ShaderConstantBuffers& GetConstantBuffers(
        const ExtractedReflection* reflection);

    static RenderPhase InferPhase(const ShaderRTBindings& bindings);

    static xr_vector<const char*> GetPhaseRTNames(RenderPhase phase);

    static ShaderRTBindings::RTSemantic InferRTSemantic(
        u32 slot,
        u32 componentMask,
        RenderPhase phase);

private:
    static VertexInputSignature AnalyzeVertexShader(slang::ShaderReflection* reflection);
    static ShaderRTBindings AnalyzePixelShader(slang::ShaderReflection* reflection);
    static ShaderConstantBuffers AnalyzeConstantBuffers(slang::ShaderReflection* reflection);

    static bool MatchesPattern(const char* name, const char* pattern);
};

}
