#include "stdafx.h"
#include "ShaderReflection.h"

namespace xray::render::framegraph {

ShaderRTBindings ShaderReflector::AnalyzePixelShader(
    ID3D11PixelShader* ps,
    ID3DBlob* bytecode) {

    VERIFY(ps);
    VERIFY(bytecode);

    ShaderRTBindings bindings;

    ID3D11ShaderReflection* reflection = nullptr;
    HRESULT hr = D3DReflect(
        bytecode->GetBufferPointer(),
        bytecode->GetBufferSize(),
        IID_ID3D11ShaderReflection,
        (void**)&reflection
    );

    if (FAILED(hr)) {
        Msg("! [ShaderReflector] Failed to create reflection interface (HRESULT: 0x%08X)", hr);
        return bindings;
    }

    D3D11_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);

    Msg("! [ShaderReflector] Analyzing pixel shader...");
    Msg("!   Bound resources: %u", shaderDesc.BoundResources);
    Msg("!   Output parameters: %u", shaderDesc.OutputParameters);

    for (u32 i = 0; i < shaderDesc.BoundResources; i++) {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        reflection->GetResourceBindingDesc(i, &bindDesc);

        if (bindDesc.Type == D3D_SIT_TEXTURE) {
            ShaderRTBindings::InputTexture input;
            input.name = bindDesc.Name;
            input.slot = bindDesc.BindPoint;

            bindings.inputTextures.push_back(input);

            Msg("!   Input texture: %s (slot t%u)",
                input.name.c_str(), input.slot);
        }
    }

    for (u32 i = 0; i < shaderDesc.OutputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        reflection->GetOutputParameterDesc(i, &paramDesc);

        if (xr_strcmp(paramDesc.SemanticName, "SV_Target") == 0) {
            ShaderRTBindings::OutputRT output;
            output.slot = paramDesc.SemanticIndex;

            bindings.outputRTs.push_back(output);

            Msg("!   Output RT: SV_Target%u", output.slot);

        } else if (xr_strcmp(paramDesc.SemanticName, "SV_Depth") == 0) {
            bindings.hasDepthOutput = true;
            Msg("!   Output: SV_Depth");
        }
    }

    bindings.phase = InferPhase(bindings);

    const char* phaseName = "Custom";
    switch (bindings.phase) {
        case RenderPhase::Geometry: phaseName = "Geometry"; break;
        case RenderPhase::Lighting: phaseName = "Lighting"; break;
        case RenderPhase::Combine: phaseName = "Combine"; break;
        case RenderPhase::PostProcess: phaseName = "PostProcess"; break;
        case RenderPhase::Shadow: phaseName = "Shadow"; break;
        default: break;
    }

    Msg("! [ShaderReflector] Inferred phase: %s", phaseName);

    reflection->Release();

    return bindings;
}

RenderPhase ShaderReflector::InferPhase(const ShaderRTBindings& bindings) {

    bool readsGBuffer = false;
    for (const auto& input : bindings.inputTextures) {
        if (MatchesPattern(input.name.c_str(), "s_position") ||
            MatchesPattern(input.name.c_str(), "s_pos") ||
            MatchesPattern(input.name.c_str(), "s_normal") ||
            MatchesPattern(input.name.c_str(), "s_diffuse") ||
            MatchesPattern(input.name.c_str(), "s_image")) {
            readsGBuffer = true;
            break;
        }
    }

    if (readsGBuffer) {
        for (const auto& input : bindings.inputTextures) {
            if (MatchesPattern(input.name.c_str(), "s_accumulator")) {
                return RenderPhase::Combine;
            }
        }

        return RenderPhase::Lighting;
    }

    if (bindings.outputRTs.size() > 1) {
        return RenderPhase::Geometry;
    }

    if (bindings.hasDepthOutput && bindings.outputRTs.empty()) {
        return RenderPhase::Shadow;
    }

    return RenderPhase::Custom;
}

bool ShaderReflector::MatchesPattern(const char* name, const char* pattern) {
    shared_str nameLower = name;
    shared_str patternLower = pattern;

    xr_string nameStr = name;
    xr_string patternStr = pattern;

    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    std::transform(patternStr.begin(), patternStr.end(), patternStr.begin(), ::tolower);

    return nameStr.find(patternStr) != xr_string::npos;
}

xr_vector<const char*> ShaderReflector::GetPhaseRTNames(RenderPhase phase) {
    switch (phase) {
        case RenderPhase::Geometry:
            return {"rt_Position", "rt_Normal", "rt_Albedo"};

        case RenderPhase::Lighting:
            return {"rt_Accumulator"};

        case RenderPhase::Combine:
            return {"rt_Generic0", "rt_Generic1"};

        case RenderPhase::Shadow:
            return {"rt_ShadowMap"};

        default:
            return {};
    }
}

}
