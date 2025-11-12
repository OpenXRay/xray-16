#include "stdafx.h"
#include "ShaderReflection.h"

namespace xray::render::framegraph {

nvrhi::Format GetFormatFromSignature(const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc) {
    u32 componentCount = 0;
    if (paramDesc.Mask & 0x1) componentCount++;
    if (paramDesc.Mask & 0x2) componentCount++;
    if (paramDesc.Mask & 0x4) componentCount++;
    if (paramDesc.Mask & 0x8) componentCount++;

    switch (paramDesc.ComponentType) {
    case D3D_REGISTER_COMPONENT_UINT32:
        switch (componentCount) {
        case 1: return nvrhi::Format::R32_UINT;
        case 2: return nvrhi::Format::RG32_UINT;
        case 3: return nvrhi::Format::RGB32_UINT;
        case 4: return nvrhi::Format::RGBA32_UINT;
        }
        break;

    case D3D_REGISTER_COMPONENT_SINT32:
        switch (componentCount) {
        case 1: return nvrhi::Format::R32_SINT;
        case 2: return nvrhi::Format::RG32_SINT;
        case 3: return nvrhi::Format::RGB32_SINT;
        case 4: return nvrhi::Format::RGBA32_SINT;
        }
        break;

    case D3D_REGISTER_COMPONENT_FLOAT32:
        switch (componentCount) {
        case 1: return nvrhi::Format::R32_FLOAT;
        case 2: return nvrhi::Format::RG32_FLOAT;
        case 3: return nvrhi::Format::RGB32_FLOAT;
        case 4: return nvrhi::Format::RGBA32_FLOAT;
        }
        break;
    }

    Msg("! [ShaderReflector] Unknown component type %d with %d components",
        paramDesc.ComponentType, componentCount);
    return nvrhi::Format::UNKNOWN;
}
VertexInputSignature ShaderReflector::AnalyzeVertexShader(
    ID3D11VertexShader* vs,
    const void* bytecode,
    size_t bytecodeSize) {

    VERIFY(bytecode);
    VERIFY(bytecodeSize > 0);

    VertexInputSignature signature;

    ID3D11ShaderReflection* reflection = nullptr;
    HRESULT hr = D3DReflect(
        bytecode,
        bytecodeSize,
        IID_ID3D11ShaderReflection,
        (void**)&reflection
    );

    if (FAILED(hr)) {
        Msg("! [ShaderReflector] Failed to create VS reflection interface (HRESULT: 0x%08X)", hr);
        return signature;
    }

    D3D11_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);

    for (u32 i = 0; i < shaderDesc.InputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        hr = reflection->GetInputParameterDesc(i, &paramDesc);

        if (FAILED(hr)) {
            Msg("! [ShaderReflector] Failed to get input parameter %u", i);
            continue;
        }

        if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED) {
            Msg("!   Skipping system value: %s (SV type: %d)",
                paramDesc.SemanticName, paramDesc.SystemValueType);
            continue;
        }

        VertexInputSignature::InputElement elem;
        elem.semanticName = paramDesc.SemanticName;
        elem.semanticIndex = paramDesc.SemanticIndex;
        elem.format = GetFormatFromSignature(paramDesc);
        elem.inputSlot = 0;

        signature.elements.push_back(elem);
    }

    reflection->Release();

    return signature;
}

ShaderConstantBuffers ShaderReflector::AnalyzeConstantBuffers(
    const void* bytecode,
    size_t bytecodeSize) {
    VERIFY(bytecode);
    VERIFY(bytecodeSize > 0);

    ShaderConstantBuffers result;

    ID3D11ShaderReflection* reflection = nullptr;
    HRESULT hr = D3DReflect(
        bytecode,
        bytecodeSize,
        IID_ID3D11ShaderReflection,
        (void**)&reflection
    );

    if (FAILED(hr)) {
        Msg("! [ShaderReflector] Failed to create reflection interface for CB analysis (HRESULT: 0x%08X)", hr);
        return result;
    }

    D3D11_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);

    for (u32 i = 0; i < shaderDesc.ConstantBuffers; i++) {
        ID3D11ShaderReflectionConstantBuffer* cbReflection = reflection->GetConstantBufferByIndex(i);
        if (!cbReflection) {
            Msg("! [ShaderReflector] Failed to get CB at index %u", i);
            continue;
        }

        D3D11_SHADER_BUFFER_DESC cbDesc;
        hr = cbReflection->GetDesc(&cbDesc);
        if (FAILED(hr)) {
            Msg("! [ShaderReflector] Failed to get CB desc at index %u", i);
            continue;
        }

        u32 bindPoint = 0;
        bool foundBinding = false;

        for (u32 j = 0; j < shaderDesc.BoundResources; j++) {
            D3D11_SHADER_INPUT_BIND_DESC bindDesc;
            hr = reflection->GetResourceBindingDesc(j, &bindDesc);
            if (FAILED(hr)) continue;

            if (bindDesc.Type == D3D_SIT_CBUFFER && xr_strcmp(bindDesc.Name, cbDesc.Name) == 0) {
                bindPoint = bindDesc.BindPoint;
                foundBinding = true;
                break;
            }
        }

        if (!foundBinding) {
            Msg("! [ShaderReflector] Warning: Could not find binding point for CB '%s'", cbDesc.Name);
        }

        ConstantBufferInfo cbInfo(cbDesc.Name, bindPoint, cbDesc.Size);
        result.buffers.push_back(cbInfo);
    }

    reflection->Release();

    return result;
}

ShaderRTBindings ShaderReflector::AnalyzePixelShader(
    ID3D11PixelShader* ps,
    const void* bytecode,
    size_t bytecodeSize) {

    VERIFY(bytecode);
    VERIFY(bytecodeSize > 0);

    ShaderRTBindings bindings;

    ID3D11ShaderReflection* reflection = nullptr;
    HRESULT hr = D3DReflect(
        bytecode,
        bytecodeSize,
        IID_ID3D11ShaderReflection,
        (void**)&reflection
    );

    if (FAILED(hr)) {
        Msg("! [ShaderReflector] Failed to create reflection interface (HRESULT: 0x%08X)", hr);
        return bindings;
    }

    D3D11_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);

    for (u32 i = 0; i < shaderDesc.BoundResources; i++) {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        reflection->GetResourceBindingDesc(i, &bindDesc);

        if (bindDesc.Type == D3D_SIT_TEXTURE) {
            ShaderRTBindings::InputTexture input;
            input.name = bindDesc.Name;
            input.slot = bindDesc.BindPoint;

            bindings.inputTextures.push_back(input);
        }
        else if (bindDesc.Type == D3D_SIT_SAMPLER) {
            ShaderRTBindings::Sampler sampler;
            sampler.name = bindDesc.Name;
            sampler.slot = bindDesc.BindPoint;

            bindings.samplers.push_back(sampler);
        }
    }

    for (u32 i = 0; i < shaderDesc.OutputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        reflection->GetOutputParameterDesc(i, &paramDesc);

        if (xr_strcmp(paramDesc.SemanticName, "SV_Target") == 0) {
            ShaderRTBindings::OutputRT output;
            output.slot = paramDesc.SemanticIndex;

            bindings.outputRTs.push_back(output);
        } else if (xr_strcmp(paramDesc.SemanticName, "SV_Depth") == 0) {
            bindings.hasDepthOutput = true;
        }
    }

    bindings.phase = InferPhase(bindings);

    for (u32 i = 0; i < shaderDesc.OutputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        reflection->GetOutputParameterDesc(i, &paramDesc);

        if (xr_strcmp(paramDesc.SemanticName, "SV_Target") == 0) {
            u32 slot = paramDesc.SemanticIndex;

            for (auto& output : bindings.outputRTs) {
                if (output.slot == slot) {
                    output.semantic = InferRTSemantic(slot, paramDesc.Mask, bindings.phase);
                    break;
                }
            }
        }
    }

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

ShaderRTBindings::RTSemantic ShaderReflector::InferRTSemantic(
    u32 slot,
    u32 componentMask,
    RenderPhase phase)
{

    if (phase == RenderPhase::Geometry) {
        switch (slot) {
            case 0:  return ShaderRTBindings::RTSemantic::Normal;
            case 1:  return ShaderRTBindings::RTSemantic::Albedo;
            case 2:  return ShaderRTBindings::RTSemantic::Material;
            default: return ShaderRTBindings::RTSemantic::Unknown;
        }
    }

    if (phase == RenderPhase::Lighting) {
        return ShaderRTBindings::RTSemantic::Accumulator;
    }

    return ShaderRTBindings::RTSemantic::Unknown;
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
