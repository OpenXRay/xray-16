// xrRender/FrameGraph/ShaderReflection.cpp
#include "stdafx.h"
#include "ShaderReflection.h"

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  ANALYZE VERTEX SHADER INPUT SIGNATURE
// ═══════════════════════════════════════════════════

nvrhi::Format GetFormatFromSignature(const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc) {
    // Count components from mask
    u32 componentCount = 0;
    if (paramDesc.Mask & 0x1) componentCount++;  // x
    if (paramDesc.Mask & 0x2) componentCount++;  // y
    if (paramDesc.Mask & 0x4) componentCount++;  // z
    if (paramDesc.Mask & 0x8) componentCount++;  // w

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

    // vs is unused - we only need bytecode for D3DReflect
    // VERIFY(vs);
    VERIFY(bytecode);
    VERIFY(bytecodeSize > 0);

    VertexInputSignature signature;

    // ═══════════════════════════════════════════════════
    //  GET SHADER REFLECTION INTERFACE
    // ═══════════════════════════════════════════════════

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

    // ═══════════════════════════════════════════════════
    //  ENUMERATE INPUT PARAMETERS (IN SHADER ORDER!)
    // ═══════════════════════════════════════════════════
    //
    // CRITICAL: GetInputParameterDesc returns parameters in the EXACT order
    // the shader expects them. We MUST preserve this order when creating
    // the D3D11 input layout, otherwise vertex data will be mismatched!

    for (u32 i = 0; i < shaderDesc.InputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        hr = reflection->GetInputParameterDesc(i, &paramDesc);

        if (FAILED(hr)) {
            Msg("! [ShaderReflector] Failed to get input parameter %u", i);
            continue;
        }

        // Skip system-value semantics (SV_VertexID, SV_InstanceID, etc.)
        if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED) {
            Msg("!   Skipping system value: %s (SV type: %d)",
                paramDesc.SemanticName, paramDesc.SystemValueType);
            continue;
        }

        VertexInputSignature::InputElement elem;
        elem.semanticName = paramDesc.SemanticName;
        elem.semanticIndex = paramDesc.SemanticIndex;
        elem.format = GetFormatFromSignature(paramDesc);
        elem.inputSlot = 0;  // Default to slot 0 (will be overridden from vertex decl)

        signature.elements.push_back(elem);
    }

    reflection->Release();

    return signature;
}

// ═══════════════════════════════════════════════════
//  ANALYZE CONSTANT BUFFERS
// ═══════════════════════════════════════════════════

ShaderConstantBuffers ShaderReflector::AnalyzeConstantBuffers(
    const void* bytecode,
    size_t bytecodeSize) {
    VERIFY(bytecode);
    VERIFY(bytecodeSize > 0);

    ShaderConstantBuffers result;

    // ═══════════════════════════════════════════════════
    //  GET SHADER REFLECTION INTERFACE
    // ═══════════════════════════════════════════════════

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

    // ═══════════════════════════════════════════════════
    //  ENUMERATE CONSTANT BUFFERS
    // ═══════════════════════════════════════════════════

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

        // Get the binding point for this CB
        u32 bindPoint = 0;
        bool foundBinding = false;

        // Search through bound resources to find this CB's binding point
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

// ═══════════════════════════════════════════════════
//  ANALYZE PIXEL SHADER
// ═══════════════════════════════════════════════════

ShaderRTBindings ShaderReflector::AnalyzePixelShader(
    ID3D11PixelShader* ps,
    const void* bytecode,
    size_t bytecodeSize) {

    // ps is unused - we only need bytecode for D3DReflect
    // VERIFY(ps);
    VERIFY(bytecode);
    VERIFY(bytecodeSize > 0);

    ShaderRTBindings bindings;

    // ═══════════════════════════════════════════════════
    //  GET SHADER REFLECTION INTERFACE
    // ═══════════════════════════════════════════════════

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

    // ═══════════════════════════════════════════════════
    //  ENUMERATE INPUT RESOURCES (Textures + Samplers)
    // ═══════════════════════════════════════════════════

    for (u32 i = 0; i < shaderDesc.BoundResources; i++) {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        reflection->GetResourceBindingDesc(i, &bindDesc);

        // Textures (SRVs)
        if (bindDesc.Type == D3D_SIT_TEXTURE) {
            ShaderRTBindings::InputTexture input;
            input.name = bindDesc.Name;  // "s_position", "s_normal", etc.
            input.slot = bindDesc.BindPoint;  // t0, t1, ...

            bindings.inputTextures.push_back(input);
        }
        // Samplers
        else if (bindDesc.Type == D3D_SIT_SAMPLER) {
            ShaderRTBindings::Sampler sampler;
            sampler.name = bindDesc.Name;  // "smp_base", "smp_rtlinear", etc.
            sampler.slot = bindDesc.BindPoint;  // s0, s1, ...

            bindings.samplers.push_back(sampler);
        }
    }

    // ═══════════════════════════════════════════════════
    //  ENUMERATE OUTPUT PARAMETERS (Render Targets)
    // ═══════════════════════════════════════════════════

    for (u32 i = 0; i < shaderDesc.OutputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        reflection->GetOutputParameterDesc(i, &paramDesc);

        if (xr_strcmp(paramDesc.SemanticName, "SV_Target") == 0) {
            ShaderRTBindings::OutputRT output;
            output.slot = paramDesc.SemanticIndex;  // SV_Target0, SV_Target1, ...

            bindings.outputRTs.push_back(output);
        } else if (xr_strcmp(paramDesc.SemanticName, "SV_Depth") == 0) {
            bindings.hasDepthOutput = true;
        }
    }

    // ═══════════════════════════════════════════════════
    //  INFER RENDER PHASE
    // ═══════════════════════════════════════════════════

    bindings.phase = InferPhase(bindings);

    // ═══════════════════════════════════════════════════
    //  INFER RT SEMANTICS BASED ON PHASE AND SLOT ORDER
    // ═══════════════════════════════════════════════════

    // Get output parameters again to extract component masks
    for (u32 i = 0; i < shaderDesc.OutputParameters; i++) {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        reflection->GetOutputParameterDesc(i, &paramDesc);

        if (xr_strcmp(paramDesc.SemanticName, "SV_Target") == 0) {
            u32 slot = paramDesc.SemanticIndex;

            // Find the OutputRT we created earlier
            for (auto& output : bindings.outputRTs) {
                if (output.slot == slot) {
                    output.semantic = InferRTSemantic(slot, paramDesc.Mask, bindings.phase);
                    break;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════
    //  CLEANUP
    // ═══════════════════════════════════════════════════

    reflection->Release();

    return bindings;
}

// ═══════════════════════════════════════════════════
//  INFER PHASE FROM BINDINGS
// ═══════════════════════════════════════════════════

RenderPhase ShaderReflector::InferPhase(const ShaderRTBindings& bindings) {
    // ═══════════════════════════════════════════════════
    //  HEURISTIC 1: Reads from GBuffer → Lighting Phase
    // ═══════════════════════════════════════════════════

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
        // If reads accumulator too → Combine phase
        for (const auto& input : bindings.inputTextures) {
            if (MatchesPattern(input.name.c_str(), "s_accumulator")) {
                return RenderPhase::Combine;
            }
        }

        return RenderPhase::Lighting;
    }

    // ═══════════════════════════════════════════════════
    //  HEURISTIC 2: Multiple Outputs → Geometry Phase
    // ═══════════════════════════════════════════════════

    if (bindings.outputRTs.size() > 1) {
        return RenderPhase::Geometry;
    }

    // ═══════════════════════════════════════════════════
    //  HEURISTIC 3: Single Output + Depth → Shadow Phase
    // ═══════════════════════════════════════════════════

    if (bindings.hasDepthOutput && bindings.outputRTs.empty()) {
        return RenderPhase::Shadow;
    }

    // ═══════════════════════════════════════════════════
    //  DEFAULT: Custom Phase
    // ═══════════════════════════════════════════════════

    return RenderPhase::Custom;
}

// ═══════════════════════════════════════════════════
//  MATCH PATTERN (Case-Insensitive Substring)
// ═══════════════════════════════════════════════════

bool ShaderReflector::MatchesPattern(const char* name, const char* pattern) {
    // Simple case-insensitive substring match
    shared_str nameLower = name;
    shared_str patternLower = pattern;

    // Convert to lowercase (X-Ray's shared_str doesn't have make_lower, use xr_strlwr)
    xr_string nameStr = name;
    xr_string patternStr = pattern;

    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    std::transform(patternStr.begin(), patternStr.end(), patternStr.begin(), ::tolower);

    return nameStr.find(patternStr) != xr_string::npos;
}

// ═══════════════════════════════════════════════════
//  INFER RT SEMANTIC FROM SLOT
// ═══════════════════════════════════════════════════

ShaderRTBindings::RTSemantic ShaderReflector::InferRTSemantic(
    u32 slot,
    u32 componentMask,
    RenderPhase phase)
{
    // ═══════════════════════════════════════════════════
    //  GEOMETRY PHASE: Use X-Ray convention
    // ═══════════════════════════════════════════════════
    // Based on vanilla X-Ray (validated from RenderDoc):
    // - Slot 0 → Normal (world/view space normal vector)
    // - Slot 1 → Albedo (base color/diffuse)
    // - Slot 2 → Material (metallic, roughness, AO, etc.)

    if (phase == RenderPhase::Geometry) {
        switch (slot) {
            case 0:  return ShaderRTBindings::RTSemantic::Normal;
            case 1:  return ShaderRTBindings::RTSemantic::Albedo;
            case 2:  return ShaderRTBindings::RTSemantic::Material;
            default: return ShaderRTBindings::RTSemantic::Unknown;
        }
    }

    // ═══════════════════════════════════════════════════
    //  LIGHTING PHASE: Accumulator buffer
    // ═══════════════════════════════════════════════════

    if (phase == RenderPhase::Lighting) {
        return ShaderRTBindings::RTSemantic::Accumulator;
    }

    // ═══════════════════════════════════════════════════
    //  OTHER PHASES: Unknown
    // ═══════════════════════════════════════════════════

    return ShaderRTBindings::RTSemantic::Unknown;
}

// ═══════════════════════════════════════════════════
//  GET PHASE RT NAMES
// ═══════════════════════════════════════════════════

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

} // namespace xray::render::framegraph
