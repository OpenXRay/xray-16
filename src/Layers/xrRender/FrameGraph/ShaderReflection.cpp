// xrRender/FrameGraph/ShaderReflection.cpp
#include "stdafx.h"
#include "ShaderReflection.h"

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  ANALYZE VERTEX SHADER INPUT SIGNATURE
// ═══════════════════════════════════════════════════

VertexInputSignature ShaderReflector::AnalyzeVertexShader(
    ID3D11VertexShader* vs,
    ID3DBlob* bytecode) {

    VERIFY(vs);
    VERIFY(bytecode);

    VertexInputSignature signature;

    // ═══════════════════════════════════════════════════
    //  GET SHADER REFLECTION INTERFACE
    // ═══════════════════════════════════════════════════

    ID3D11ShaderReflection* reflection = nullptr;
    HRESULT hr = D3DReflect(
        bytecode->GetBufferPointer(),
        bytecode->GetBufferSize(),
        IID_ID3D11ShaderReflection,
        (void**)&reflection
    );

    if (FAILED(hr)) {
        Msg("! [ShaderReflector] Failed to create VS reflection interface (HRESULT: 0x%08X)", hr);
        return signature;
    }

    D3D11_SHADER_DESC shaderDesc;
    reflection->GetDesc(&shaderDesc);

    Msg("! [ShaderReflector] Analyzing vertex shader input signature...");
    Msg("!   Input parameters: %u", shaderDesc.InputParameters);

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

        // Infer DXGI format from component type and mask
        // This is a simplified version - real impl needs more cases
        if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
            u32 numComponents = 0;
            if (paramDesc.Mask & 0x1) numComponents++;
            if (paramDesc.Mask & 0x2) numComponents++;
            if (paramDesc.Mask & 0x4) numComponents++;
            if (paramDesc.Mask & 0x8) numComponents++;

            switch (numComponents) {
                case 1: elem.format = DXGI_FORMAT_R32_FLOAT; break;
                case 2: elem.format = DXGI_FORMAT_R32G32_FLOAT; break;
                case 3: elem.format = DXGI_FORMAT_R32G32B32_FLOAT; break;
                case 4: elem.format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                default: elem.format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
            }
        } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
            // Handle UINT formats (used for packed data)
            elem.format = DXGI_FORMAT_R32G32B32A32_UINT;
        } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
            // Handle SINT formats (used for indices, short texcoords)
            elem.format = DXGI_FORMAT_R32G32B32A32_SINT;
        } else {
            // Unknown - default to float4
            elem.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        elem.inputSlot = 0;  // Default to slot 0 (will be overridden from vertex decl)

        signature.elements.push_back(elem);

        Msg("!   Input[%u]: %s%d (format inferred: %d, mask: 0x%X)",
            i, elem.semanticName.c_str(), elem.semanticIndex, elem.format, paramDesc.Mask);
    }

    reflection->Release();

    Msg("! [ShaderReflector] Extracted %u input elements in shader order", signature.elements.size());
    return signature;
}

// ═══════════════════════════════════════════════════
//  ANALYZE PIXEL SHADER
// ═══════════════════════════════════════════════════

ShaderRTBindings ShaderReflector::AnalyzePixelShader(
    ID3D11PixelShader* ps,
    ID3DBlob* bytecode) {

    VERIFY(ps);
    VERIFY(bytecode);

    ShaderRTBindings bindings;

    // ═══════════════════════════════════════════════════
    //  GET SHADER REFLECTION INTERFACE
    // ═══════════════════════════════════════════════════

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

            Msg("!   Input texture: %s (slot t%u)",
                input.name.c_str(), input.slot);
        }
        // Samplers
        else if (bindDesc.Type == D3D_SIT_SAMPLER) {
            ShaderRTBindings::Sampler sampler;
            sampler.name = bindDesc.Name;  // "smp_base", "smp_rtlinear", etc.
            sampler.slot = bindDesc.BindPoint;  // s0, s1, ...

            bindings.samplers.push_back(sampler);

            Msg("!   Sampler: %s (slot s%u)",
                sampler.name.c_str(), sampler.slot);
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

            Msg("!   Output RT: SV_Target%u", output.slot);

        } else if (xr_strcmp(paramDesc.SemanticName, "SV_Depth") == 0) {
            bindings.hasDepthOutput = true;
            Msg("!   Output: SV_Depth");
        }
    }

    // ═══════════════════════════════════════════════════
    //  INFER RENDER PHASE
    // ═══════════════════════════════════════════════════

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

                    const char* semanticName = "Unknown";
                    switch (output.semantic) {
                        case ShaderRTBindings::RTSemantic::Normal: semanticName = "Normal"; break;
                        case ShaderRTBindings::RTSemantic::Albedo: semanticName = "Albedo"; break;
                        case ShaderRTBindings::RTSemantic::Material: semanticName = "Material"; break;
                        case ShaderRTBindings::RTSemantic::Position: semanticName = "Position"; break;
                        case ShaderRTBindings::RTSemantic::Emissive: semanticName = "Emissive"; break;
                        case ShaderRTBindings::RTSemantic::Accumulator: semanticName = "Accumulator"; break;
                        default: break;
                    }

                    Msg("!   → Slot %u semantic: %s", slot, semanticName);
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
