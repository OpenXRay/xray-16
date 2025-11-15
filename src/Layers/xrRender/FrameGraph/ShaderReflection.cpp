// xrRender/FrameGraph/ShaderReflection.cpp
#include "stdafx.h"
#include "ShaderReflection.h"
#include "ShaderCache.h"  // For ExtractedReflection definition
#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include <slang.h>

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  ANALYZE VERTEX SHADER INPUT SIGNATURE
// ═══════════════════════════════════════════════════

// Helper: Convert Slang type to NVRHI format
nvrhi::Format GetFormatFromSlangType(slang::TypeReflection* type) {
    if (!type)
        return nvrhi::Format::UNKNOWN;

    auto scalarType = type->getScalarType();
    auto kind = type->getKind();

    // Handle vector types
    if (kind == slang::TypeReflection::Kind::Vector) {
        u32 elementCount = type->getElementCount();

        switch (scalarType) {
        case slang::TypeReflection::ScalarType::Float32:
            switch (elementCount) {
            case 1: return nvrhi::Format::R32_FLOAT;
            case 2: return nvrhi::Format::RG32_FLOAT;
            case 3: return nvrhi::Format::RGB32_FLOAT;
            case 4: return nvrhi::Format::RGBA32_FLOAT;
            }
            break;

        case slang::TypeReflection::ScalarType::UInt32:
            switch (elementCount) {
            case 1: return nvrhi::Format::R32_UINT;
            case 2: return nvrhi::Format::RG32_UINT;
            case 3: return nvrhi::Format::RGB32_UINT;
            case 4: return nvrhi::Format::RGBA32_UINT;
            }
            break;

        case slang::TypeReflection::ScalarType::Int32:
            switch (elementCount) {
            case 1: return nvrhi::Format::R32_SINT;
            case 2: return nvrhi::Format::RG32_SINT;
            case 3: return nvrhi::Format::RGB32_SINT;
            case 4: return nvrhi::Format::RGBA32_SINT;
            }
            break;
        }
    }
    // Handle scalar types (treat as 1-component vector)
    else if (kind == slang::TypeReflection::Kind::Scalar) {
        switch (scalarType) {
        case slang::TypeReflection::ScalarType::Float32: return nvrhi::Format::R32_FLOAT;
        case slang::TypeReflection::ScalarType::UInt32: return nvrhi::Format::R32_UINT;
        case slang::TypeReflection::ScalarType::Int32: return nvrhi::Format::R32_SINT;
        }
    }

    Msg("! [ShaderReflector] Unsupported Slang type: kind=%d, scalarType=%d",
        (int)kind, (int)scalarType);
    return nvrhi::Format::UNKNOWN;
}
VertexInputSignature ShaderReflector::AnalyzeVertexShader(
    slang::ShaderReflection* reflection) {

    VertexInputSignature signature;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for vertex shader");
        return signature;
    }

    // ═══════════════════════════════════════════════════
    //  GET ENTRY POINT AND DRILL INTO INPUT STRUCT
    // ═══════════════════════════════════════════════════
    // With getProgramWithEntryPoints(), vertex inputs are inside the entry
    // point's input parameter struct. We need to get the entry point,
    // find its input parameter, and enumerate the struct's fields.

    if (reflection->getEntryPointCount() == 0) {
        Msg("! [ShaderReflector] No entry points found in vertex shader");
        return signature;
    }

    auto* entryPoint = reflection->getEntryPointByIndex(0);
    if (!entryPoint) {
        Msg("! [ShaderReflector] Failed to get vertex shader entry point");
        return signature;
    }

    // Find the VaryingInput parameter (the input struct)
    u32 paramCount = entryPoint->getParameterCount();
    Msg("  [ShaderReflector] Entry point has %u parameters", paramCount);

    slang::VariableLayoutReflection* inputParam = nullptr;
    for (u32 i = 0; i < paramCount; i++) {
        auto* param = entryPoint->getParameterByIndex(i);
        if (!param) continue;

        auto* typeLayout = param->getTypeLayout();
        if (!typeLayout) continue;

        if (typeLayout->getParameterCategory() == slang::ParameterCategory::VaryingInput) {
            inputParam = param;
            Msg("  [ShaderReflector] Found input parameter at index %u: '%s'", i, param->getName());
            break;
        }
    }

    if (!inputParam) {
        Msg("! [ShaderReflector] No VaryingInput parameter found in entry point");
        return signature;
    }

    // Get the type of the input parameter (should be a struct)
    auto* inputTypeLayout = inputParam->getTypeLayout();
    auto* inputType = inputTypeLayout->getType();
    if (!inputType) {
        Msg("! [ShaderReflector] Input parameter has no type");
        return signature;
    }

    // Enumerate struct fields (these are the actual vertex inputs with semantics)
    u32 fieldCount = inputType->getFieldCount();
    Msg("  [ShaderReflector] Input struct has %u fields", fieldCount);

    for (u32 i = 0; i < fieldCount; i++) {
        auto* field = inputType->getFieldByIndex(i);
        auto* fieldLayout = inputTypeLayout->getFieldByIndex(i);

        if (!field || !fieldLayout)
            continue;

        const char* fieldName = field->getName() ? field->getName() : "<unknown>";

        // Get semantic name (POSITION, TEXCOORD, etc.)
        const char* semanticName = fieldLayout->getSemanticName();
        Msg("  [ShaderReflector] Field[%u]: name='%s', semantic='%s'",
            i, fieldName, semanticName ? semanticName : "<none>");

        if (!semanticName || semanticName[0] == '\0') {
            Msg("! [ShaderReflector] Skipping field[%u] '%s' - no semantic name", i, fieldName);
            continue;
        }

        // Parse semantic index
        u32 semanticIndex = fieldLayout->getSemanticIndex();

        // Get format from field type
        auto* fieldTypeLayout = fieldLayout->getTypeLayout();
        auto* fieldType = fieldTypeLayout ? fieldTypeLayout->getType() : nullptr;
        nvrhi::Format format = GetFormatFromSlangType(fieldType);

        VertexInputSignature::InputElement elem;
        elem.semanticName = semanticName;
        elem.semanticIndex = semanticIndex;
        elem.format = format;
        elem.inputSlot = 0;  // Default to slot 0 (will be overridden from vertex decl)

        signature.elements.push_back(elem);
    }

    // NOTE: Don't release reflection - it's owned by the shader struct (SVS/SPS)

    return signature;
}

// ═══════════════════════════════════════════════════
//  ANALYZE CONSTANT BUFFERS
// ═══════════════════════════════════════════════════

ShaderConstantBuffers ShaderReflector::AnalyzeConstantBuffers(
    slang::ShaderReflection* reflection) {

    ShaderConstantBuffers result;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for constant buffer analysis");
        return result;
    }

    // ═══════════════════════════════════════════════════
    //  USE SLANG REFLECTION WRAPPER
    // ═══════════════════════════════════════════════════
    //
    // Much simpler than D3DReflect! Slang gives us bind points directly.

    SlangReflectionWrapper wrapper(reflection);
    auto constantBuffers = wrapper.GetConstantBuffers();

    for (const auto& cb : constantBuffers) {
        ConstantBufferInfo info;
        info.name = cb.name;
        info.slot = cb.slot;  // Direct from Slang - no searching needed!
        info.size = cb.size;

        result.buffers.push_back(info);
    }

    // NOTE: Don't release reflection - it's owned by the shader struct (SVS/SPS)

    return result;
}

// ═══════════════════════════════════════════════════
//  ANALYZE PIXEL SHADER
// ═══════════════════════════════════════════════════

ShaderRTBindings ShaderReflector::AnalyzePixelShader(
    slang::ShaderReflection* reflection) {

    ShaderRTBindings bindings;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for pixel shader");
        return bindings;
    }

    // ═══════════════════════════════════════════════════
    //  EXTRACT INPUT RESOURCES (Textures + Samplers)
    // ═══════════════════════════════════════════════════
    //
    // Much simpler with Slang! Just use the wrapper.

    SlangReflectionWrapper wrapper(reflection);

    // Input textures (SRVs)
    auto textures = wrapper.GetTextures();
    for (const auto& tex : textures) {
        ShaderRTBindings::InputTexture input;
        input.name = tex.name;
        input.slot = tex.slot;
        bindings.inputTextures.push_back(input);
    }

    // Samplers
    auto samplers = wrapper.GetSamplers();
    for (const auto& samp : samplers) {
        ShaderRTBindings::Sampler sampler;
        sampler.name = samp.name;
        sampler.slot = samp.slot;
        bindings.samplers.push_back(sampler);
    }

    // ═══════════════════════════════════════════════════
    //  ENUMERATE OUTPUT PARAMETERS (Render Targets)
    // ═══════════════════════════════════════════════════

    if (reflection->getEntryPointCount() == 0) {
        Msg("! [ShaderReflector] No entry points found in pixel shader");
        return bindings;
    }

    auto* entryPoint = reflection->getEntryPointByIndex(0);
    if (!entryPoint) {
        Msg("! [ShaderReflector] Failed to get pixel shader entry point");
        return bindings;
    }

    // ═══════════════════════════════════════════════════
    //  CHECK RETURN TYPE FOR RENDER TARGETS
    // ═══════════════════════════════════════════════════
    // In HLSL/Slang, pixel shader outputs are typically the return value,
    // not parameters. We need to examine the return type's fields.

    auto* resultVarLayout = entryPoint->getResultVarLayout();
    if (resultVarLayout)
    {
        auto* typeLayout = resultVarLayout->getTypeLayout();
        if (typeLayout)
        {
            auto* type = typeLayout->getType();
            if (type && type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                // Return type is a struct - enumerate its fields for render targets
                u32 fieldCount = type->getFieldCount();
                Msg("  [AnalyzePixelShader] Return struct has %u fields", fieldCount);

                for (u32 i = 0; i < fieldCount; i++)
                {
                    auto* field = type->getFieldByIndex(i);
                    if (!field)
                        continue;

                    const char* fieldName = field->getName();

                    // Get semantic from the field's variable layout in the struct
                    auto* fieldLayout = typeLayout->getFieldByIndex(i);
                    if (!fieldLayout)
                        continue;

                    const char* semantic = fieldLayout->getSemanticName();
                    u32 semanticIndex = fieldLayout->getSemanticIndex();

                    Msg("  [AnalyzePixelShader] Field[%u]: name='%s', semantic='%s', index=%u",
                        i, fieldName ? fieldName : "null", semantic ? semantic : "null", semanticIndex);

                    if (semantic)
                    {
                        // Check for SV_Target outputs
                        if (xr_strcmp(semantic, "SV_Target") == 0)
                        {
                            ShaderRTBindings::OutputRT output;
                            output.slot = semanticIndex;
                            bindings.outputRTs.push_back(output);
                            Msg("  [AnalyzePixelShader] Added RT output at slot %u", semanticIndex);
                        }
                        // Check for depth output
                        else if (xr_strcmp(semantic, "SV_Depth") == 0)
                        {
                            bindings.hasDepthOutput = true;
                            Msg("  [AnalyzePixelShader] Found depth output");
                        }
                    }
                }
            }
            else if (type)
            {
                // Return type is a simple type (e.g., float4)
                // Check if it has a semantic
                const char* semantic = resultVarLayout->getSemanticName();
                u32 semanticIndex = resultVarLayout->getSemanticIndex();

                Msg("  [AnalyzePixelShader] Simple return type: semantic='%s', index=%u",
                    semantic ? semantic : "null", semanticIndex);

                if (semantic && xr_strcmp(semantic, "SV_Target") == 0)
                {
                    ShaderRTBindings::OutputRT output;
                    output.slot = semanticIndex;
                    bindings.outputRTs.push_back(output);
                    Msg("  [AnalyzePixelShader] Added RT output at slot %u", semanticIndex);
                }
            }
        }
    }
    else
    {
        Msg("! [AnalyzePixelShader] No result var layout found");
    }

    // ═══════════════════════════════════════════════════
    //  INFER RENDER PHASE
    // ═══════════════════════════════════════════════════

    bindings.phase = InferPhase(bindings);

    // ═══════════════════════════════════════════════════
    //  INFER RT SEMANTICS BASED ON PHASE AND SLOT ORDER
    // ═══════════════════════════════════════════════════

    // Second pass: extract component masks for semantic inference from return type fields
    if (resultVarLayout)
    {
        auto* typeLayout = resultVarLayout->getTypeLayout();
        auto* type = typeLayout->getType();
        if (type && type->getKind() == slang::TypeReflection::Kind::Struct)
        {
            u32 fieldCount = type->getFieldCount();
            for (u32 i = 0; i < fieldCount; i++)
            {
                auto* fieldLayout = typeLayout->getFieldByIndex(i);
                if (!fieldLayout)
                    continue;

                const char* semantic = fieldLayout->getSemanticName();
                if (!semantic || xr_strcmp(semantic, "SV_Target") != 0)
                    continue;

                u32 slot = fieldLayout->getSemanticIndex();

                // Get component mask from field type
                auto* fieldTypeLayout = fieldLayout->getTypeLayout();
                auto* fieldType = fieldTypeLayout ? fieldTypeLayout->getType() : nullptr;
                u32 componentMask = 0;
                if (fieldType)
                {
                    auto kind = fieldType->getKind();
                    u32 componentCount = 1;

                    if (kind == slang::TypeReflection::Kind::Vector)
                    {
                        componentCount = fieldType->getElementCount();
                    }

                    // Build mask: 1=x, 2=y, 4=z, 8=w
                    for (u32 c = 0; c < componentCount && c < 4; c++)
                    {
                        componentMask |= (1 << c);
                    }
                }

                // Find the OutputRT we created earlier and set semantic
                for (auto& output : bindings.outputRTs)
                {
                    if (output.slot == slot)
                    {
                        output.semantic = InferRTSemantic(slot, componentMask, bindings.phase);
                        break;
                    }
                }
            }
        }
    }

    // NOTE: Don't release reflection - it's owned by the shader struct (SVS/SPS)

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

// ═══════════════════════════════════════════════════
//  EXTRACT REFLECTION (NEW UNIFIED API)
// ═══════════════════════════════════════════════════

ExtractedReflection ShaderReflector::ExtractReflection(
    slang::ShaderReflection* slangReflection,
    bool isVertexShader)
{
    ExtractedReflection result;

    if (!slangReflection) {
        Msg("! [ShaderReflector::ExtractReflection] NULL Slang reflection!");
        return result;
    }

    Msg("  [ShaderReflector::ExtractReflection] Extracting %s shader reflection (entryPoints=%u)",
        isVertexShader ? "vertex" : "pixel",
        slangReflection->getEntryPointCount());

    // Extract all reflection data using existing Analyze methods
    if (isVertexShader)
    {
        result.vertexInputSignature = AnalyzeVertexShader(slangReflection);
        Msg("  [ShaderReflector::ExtractReflection] Extracted %u vertex inputs",
            result.vertexInputSignature.elements.size());
    }
    else
    {
        result.rtBindings = AnalyzePixelShader(slangReflection);
        Msg("  [ShaderReflector::ExtractReflection] Extracted %u RT outputs",
            result.rtBindings.outputRTs.size());
    }

    result.constantBuffers = AnalyzeConstantBuffers(slangReflection);
    Msg("  [ShaderReflector::ExtractReflection] Extracted %u constant buffers",
        result.constantBuffers.buffers.size());

    return result;
}

// ═══════════════════════════════════════════════════
//  REFLECTION ACCESSORS
// ═══════════════════════════════════════════════════

const VertexInputSignature& ShaderReflector::GetVertexInputSignature(
    const ExtractedReflection* reflection)
{
    static VertexInputSignature empty;
    return reflection ? reflection->vertexInputSignature : empty;
}

const ShaderRTBindings& ShaderReflector::GetRTBindings(
    const ExtractedReflection* reflection)
{
    static ShaderRTBindings empty;
    return reflection ? reflection->rtBindings : empty;
}

const ShaderConstantBuffers& ShaderReflector::GetConstantBuffers(
    const ExtractedReflection* reflection)
{
    static ShaderConstantBuffers empty;
    return reflection ? reflection->constantBuffers : empty;
}

} // namespace xray::render::framegraph
