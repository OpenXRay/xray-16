#include "stdafx.h"
#include "ShaderReflection.h"
#include "ShaderCache.h"
#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include <slang.h>

namespace xray::render::framegraph {

namespace {

ShaderConstant::Type DetectConstantType(
    slang::TypeLayoutReflection* typeLayout,
    u16& outElementSize,
    u16& outMatrixStride,
    u16& outArrayCount)
{
    if (!typeLayout) {
        outElementSize = 0;
        outArrayCount = 1;
        outMatrixStride = 0;
        return ShaderConstant::Type::Struct;
    }

    auto* type = typeLayout->getType();
    if (!type) {
        outElementSize = static_cast<u16>(typeLayout->getSize());
        outArrayCount = 1;
        outMatrixStride = 0;
        return ShaderConstant::Type::Struct;
    }

    slang::TypeReflection::Kind kind = type->getKind();

    switch (kind) {
        case slang::TypeReflection::Kind::Scalar: {
            outElementSize = static_cast<u16>(typeLayout->getSize());
            outArrayCount = 1;
            outMatrixStride = 0;
            return ShaderConstant::Type::Scalar;
        }

        case slang::TypeReflection::Kind::Vector: {
            outElementSize = static_cast<u16>(typeLayout->getSize());
            outArrayCount = 1;
            outMatrixStride = 0;
            return ShaderConstant::Type::Vector;
        }

        case slang::TypeReflection::Kind::Matrix: {
            auto rowCount = type->getRowCount();
            auto colCount = type->getColumnCount();

            outMatrixStride = static_cast<u16>(typeLayout->getStride(slang::ParameterCategory::Uniform));
            outElementSize = static_cast<u16>(typeLayout->getSize());
            outArrayCount = 1;

            if (rowCount == 3 && colCount == 4) {
                return ShaderConstant::Type::Matrix3x4;
            } else if (rowCount == 4 && colCount == 4) {
                return ShaderConstant::Type::Matrix4x4;
            }

            Msg("! [DetectConstantType] Unsupported matrix dimensions: %dx%d",
                rowCount, colCount);
            return ShaderConstant::Type::Struct;
        }

        case slang::TypeReflection::Kind::Array: {
            auto* elementTypeLayout = typeLayout->getElementTypeLayout();

            outArrayCount = static_cast<u16>(typeLayout->getElementCount());
            outElementSize = elementTypeLayout ? static_cast<u16>(elementTypeLayout->getSize()) : 0;
            outMatrixStride = 0;

            if (elementTypeLayout) {
                auto* elementType = elementTypeLayout->getType();
                if (elementType && elementType->getKind() == slang::TypeReflection::Kind::Matrix) {
                    outMatrixStride = static_cast<u16>(elementTypeLayout->getStride(slang::ParameterCategory::Uniform));
                }
            }

            return ShaderConstant::Type::Array;
        }

        default: {
            outElementSize = static_cast<u16>(typeLayout->getSize());
            outArrayCount = 1;
            outMatrixStride = 0;
            return ShaderConstant::Type::Struct;
        }
    }
}

}

nvrhi::Format GetFormatFromSlangType(slang::TypeReflection* type) {
    if (!type)
        return nvrhi::Format::UNKNOWN;

    auto scalarType = type->getScalarType();
    auto kind = type->getKind();

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

    if (reflection->getEntryPointCount() == 0) {
        Msg("! [ShaderReflector] No entry points found in vertex shader");
        return signature;
    }

    auto* entryPoint = reflection->getEntryPointByIndex(0);
    if (!entryPoint) {
        Msg("! [ShaderReflector] Failed to get vertex shader entry point");
        return signature;
    }

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

    auto* inputTypeLayout = inputParam->getTypeLayout();
    auto* inputType = inputTypeLayout->getType();
    if (!inputType) {
        Msg("! [ShaderReflector] Input parameter has no type");
        return signature;
    }

    u32 fieldCount = inputType->getFieldCount();
    Msg("  [ShaderReflector] Input struct has %u fields", fieldCount);

    for (u32 i = 0; i < fieldCount; i++) {
        auto* field = inputType->getFieldByIndex(i);
        auto* fieldLayout = inputTypeLayout->getFieldByIndex(i);

        if (!field || !fieldLayout)
            continue;

        const char* fieldName = field->getName() ? field->getName() : "<unknown>";

        const char* semanticName = fieldLayout->getSemanticName();
        Msg("  [ShaderReflector] Field[%u]: name='%s', semantic='%s'",
            i, fieldName, semanticName ? semanticName : "<none>");

        if (!semanticName || semanticName[0] == '\0') {
            Msg("! [ShaderReflector] Skipping field[%u] '%s' - no semantic name", i, fieldName);
            continue;
        }

        u32 semanticIndex = fieldLayout->getSemanticIndex();

        auto* fieldTypeLayout = fieldLayout->getTypeLayout();
        auto* fieldType = fieldTypeLayout ? fieldTypeLayout->getType() : nullptr;
        nvrhi::Format format = GetFormatFromSlangType(fieldType);

        VertexInputSignature::InputElement elem;
        elem.semanticName = semanticName;
        elem.semanticIndex = semanticIndex;
        elem.format = format;
        elem.inputSlot = 0;

        signature.elements.push_back(elem);
    }

    return signature;
}

ShaderConstantBuffers ShaderReflector::AnalyzeConstantBuffers(
    slang::ShaderReflection* reflection) {

    ShaderConstantBuffers result;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for constant buffer analysis");
        return result;
    }

    SlangReflectionWrapper wrapper(reflection);
    auto constantBuffers = wrapper.GetConstantBuffers();

    for (const auto& cb : constantBuffers) {
        ConstantBufferInfo info;
        info.name = cb.name;
        info.slot = cb.slot;
        info.size = cb.size;

        result.buffers.push_back(info);
    }

    return result;
}

UpdateFrequency ShaderReflector::InferConstantFrequency(const char* name, const char* cbName) {

    if (strstr(cbName, "static_globals") != nullptr) {
        return UpdateFrequency::Engine;
    }

    if (strstr(name, "m_VP") != nullptr ||
        strstr(name, "m_V") != nullptr ||
        strstr(name, "m_P") != nullptr ||
        strstr(name, "eye_position") != nullptr ||
        strstr(name, "eye_direction") != nullptr) {
        return UpdateFrequency::Pass;
    }

    if (strstr(name, "dt_params") != nullptr ||
        strstr(name, "material") != nullptr ||
        strstr(cbName, "dynamic_transforms") != nullptr) {
        return UpdateFrequency::Material;
    }

    if (strstr(name, "m_World") != nullptr ||
        strstr(name, "m_WV") != nullptr ||
        strstr(name, "m_WVP") != nullptr ||
        strstr(name, "m_xform") != nullptr ||
        strstr(name, "bones") != nullptr ||
        strstr(cbName, "$Globals") != nullptr) {
        return UpdateFrequency::Instance;
    }

    return UpdateFrequency::Instance;
}

ConstantPersistence ShaderReflector::InferConstantPersistence(const char* cbName, UpdateFrequency frequency) {

    if (strstr(cbName, "dynamic_transforms") != nullptr ||
        strstr(cbName, "$Globals") != nullptr) {
        return ConstantPersistence::Volatile;
    }

    if (strstr(cbName, "static_globals") != nullptr) {
        return ConstantPersistence::Static;
    }

    if (frequency == UpdateFrequency::Engine || frequency == UpdateFrequency::Pass) {
        return ConstantPersistence::Static;
    }

    return ConstantPersistence::Volatile;
}

ShaderConstantLayout ShaderReflector::AnalyzeConstantLayout(slang::ShaderReflection* reflection) {
    ShaderConstantLayout layout;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for constant layout analysis");
        return layout;
    }

    layout.constantBuffers = AnalyzeConstantBuffers(reflection);

    Msg("  [AnalyzeConstantLayout] Found %u constant buffers", layout.constantBuffers.buffers.size());

    SlangReflectionWrapper wrapper(reflection);
    auto constantBuffers = wrapper.GetConstantBuffers();

    for (u32 cbIdx = 0; cbIdx < constantBuffers.size(); ++cbIdx) {
        const auto& cbInfo = constantBuffers[cbIdx];

        Msg("  [AnalyzeConstantLayout] Processing CB[%u]: '%s' (size=%u)", cbIdx, cbInfo.name, cbInfo.size);

        u32 paramCount = reflection->getParameterCount();
        Msg("  [AnalyzeConstantLayout]   Shader has %u parameters total", paramCount);

        bool foundMatchingParam = false;
        for (u32 paramIdx = 0; paramIdx < paramCount; ++paramIdx) {
            auto* param = reflection->getParameterByIndex(paramIdx);
            if (!param) {
                Msg("  [AnalyzeConstantLayout]   Param[%u]: NULL", paramIdx);
                continue;
            }

            const char* paramName = param->getName();
            Msg("  [AnalyzeConstantLayout]   Param[%u]: '%s'", paramIdx, paramName ? paramName : "NULL");

            auto* typeLayout = param->getTypeLayout();
            if (!typeLayout) {
                Msg("  [AnalyzeConstantLayout]     No typeLayout");
                continue;
            }

            auto paramCategory = typeLayout->getParameterCategory();
            if (paramCategory != slang::ParameterCategory::ConstantBuffer) {
                Msg("  [AnalyzeConstantLayout]     Not a CB (category=%d)", (int)paramCategory);
                continue;
            }

            Msg("  [AnalyzeConstantLayout]     IS a constant buffer parameter");

            if (!paramName || xr_strcmp(paramName, cbInfo.name) != 0) {
                Msg("  [AnalyzeConstantLayout]     Name mismatch: '%s' != '%s'", paramName ? paramName : "NULL", cbInfo.name);
                continue;
            }

            foundMatchingParam = true;
            Msg("  [AnalyzeConstantLayout]     MATCHED! Extracting fields...");

            auto* elementTypeLayout = typeLayout->getElementTypeLayout();
            if (!elementTypeLayout) {
                Msg("  [AnalyzeConstantLayout]     ERROR: No elementTypeLayout (CB wrapper is empty?)");
                continue;
            }

            auto* cbType = elementTypeLayout->getType();
            if (!cbType) {
                Msg("  [AnalyzeConstantLayout]     ERROR: No cbType from element");
                continue;
            }

            auto typeKind = cbType->getKind();
            Msg("  [AnalyzeConstantLayout]     Unwrapped cbType kind: %d (Struct=%d)", (int)typeKind, (int)slang::TypeReflection::Kind::Struct);

            if (typeKind != slang::TypeReflection::Kind::Struct) {
                Msg("  [AnalyzeConstantLayout]     ERROR: Not a struct type after unwrapping");
                continue;
            }

            u32 fieldCount = cbType->getFieldCount();
            Msg("  [AnalyzeConstantLayout]     Found %u fields in CB struct", fieldCount);

            for (u32 i = 0; i < fieldCount; ++i) {
                auto* field = cbType->getFieldByIndex(i);
                auto* fieldLayout = elementTypeLayout->getFieldByIndex(i);

                if (!field || !fieldLayout) continue;

                const char* fieldName = field->getName();
                if (!fieldName) continue;

                ShaderConstant constant;
                constant.name = fieldName;
                constant.offset = (u32)fieldLayout->getOffset(slang::ParameterCategory::Uniform);

                auto* fieldTypeLayout = fieldLayout->getTypeLayout();
                constant.type = DetectConstantType(
                    fieldTypeLayout,
                    constant.elementSize,
                    constant.matrixStride,
                    constant.arrayCount
                );

                constant.size = (constant.arrayCount > 1)
                    ? (constant.elementSize * constant.arrayCount)
                    : (fieldTypeLayout ? (u32)fieldTypeLayout->getSize() : 0);
                constant.cbIndex = cbIdx;

                constant.frequency = InferConstantFrequency(fieldName, cbInfo.name);

                constant.persistence = InferConstantPersistence(cbInfo.name, constant.frequency);

                layout.constants.push_back(constant);
            }
        }

        if (!foundMatchingParam) {
            Msg("  [AnalyzeConstantLayout]   WARNING: No matching parameter found for CB '%s'", cbInfo.name);
        }
    }

    Msg("  [AnalyzeConstantLayout] === STEP 2: Checking for loose constants ($Globals) ===");

    auto* globalParamsLayout = reflection->getGlobalParamsTypeLayout();

    if (!globalParamsLayout) {
        Msg("  [AnalyzeConstantLayout] No globalParamsLayout (no loose constants in shader)");
    } else {
        Msg("  [AnalyzeConstantLayout] globalParamsLayout exists, checking fields...");

        auto* globalParamsVar = reflection->getGlobalParamsVarLayout();
        u32 globalsBindingSlot = 0;

        if (globalParamsVar) {
            globalsBindingSlot = static_cast<u32>(globalParamsVar->getOffset(slang::ParameterCategory::ConstantBuffer));
            Msg("  [AnalyzeConstantLayout] $Globals binding: b%u (from reflection)", globalsBindingSlot);
        } else {
            globalsBindingSlot = 0;
            Msg("  [AnalyzeConstantLayout] $Globals binding: b0 (fallback - Slang didn't provide slot)");
        }

        u32 fieldCount = static_cast<u32>(globalParamsLayout->getFieldCount());
        Msg("  [AnalyzeConstantLayout] globalParamsLayout has %u fields", fieldCount);

        if (fieldCount > 0) {
            Msg("  [AnalyzeConstantLayout] globalParamsLayout has %u fields, checking if valid loose constants", fieldCount);

            ConstantBufferInfo implicitCB;
            implicitCB.name = "$Globals";
            implicitCB.slot = globalsBindingSlot;
            implicitCB.size = static_cast<u32>(globalParamsLayout->getSize());

            u16 cbIndex = static_cast<u16>(layout.constantBuffers.buffers.size());
            u32 extractedConstantCount = 0;

            for (u32 i = 0; i < fieldCount; ++i) {
                auto* fieldLayout = globalParamsLayout->getFieldByIndex(i);
                if (!fieldLayout) {
                    Msg("    [AnalyzeConstantLayout] Field[%u]: NULL layout", i);
                    continue;
                }

                auto* fieldTypeLayout = fieldLayout->getTypeLayout();
                if (fieldTypeLayout) {
                    auto category = fieldTypeLayout->getParameterCategory();
                    if (category != slang::ParameterCategory::Uniform) {
                        continue;
                    }
                }

                auto* fieldVar = fieldLayout->getVariable();
                if (!fieldVar) {
                    Msg("    [AnalyzeConstantLayout] Field[%u]: No variable", i);
                    continue;
                }

                const char* fieldName = fieldVar->getName();
                if (!fieldName) {
                    Msg("    [AnalyzeConstantLayout] Field[%u]: NULL name", i);
                    continue;
                }

                ShaderConstant constant;
                constant.name = fieldName;
                constant.offset = static_cast<u32>(fieldLayout->getOffset(slang::ParameterCategory::Uniform));
                constant.cbIndex = cbIndex;

                constant.type = DetectConstantType(
                    fieldTypeLayout,
                    constant.elementSize,
                    constant.matrixStride,
                    constant.arrayCount
                );

                constant.size = (constant.arrayCount > 1)
                    ? (constant.elementSize * constant.arrayCount)
                    : static_cast<u32>(fieldTypeLayout ? fieldTypeLayout->getSize() : 0);

                constant.frequency = InferConstantFrequency(fieldName, "$Globals");
                constant.persistence = ConstantPersistence::Volatile;

                layout.constants.push_back(constant);
                extractedConstantCount++;

                const char* typeStr = "unknown";
                switch (constant.type) {
                case ShaderConstant::Type::Scalar: typeStr = "scalar"; break;
                case ShaderConstant::Type::Vector: typeStr = "vector"; break;
                case ShaderConstant::Type::Matrix3x4: typeStr = "float3x4"; break;
                case ShaderConstant::Type::Matrix4x4: typeStr = "float4x4"; break;
                case ShaderConstant::Type::Array: typeStr = "array"; break;
                case ShaderConstant::Type::Struct: typeStr = "struct"; break;
                }

                Msg("    [AnalyzeConstantLayout] Loose constant: %s (type=%s, offset=%u, size=%u, freq=%d)",
                    fieldName, typeStr, constant.offset, constant.size,
                    static_cast<int>(constant.frequency));
            }

            if (extractedConstantCount > 0) {
                if (implicitCB.size == 0) {
                    u32 calculatedSize = 0;
                    for (const auto& constant : layout.constants) {
                        if (constant.cbIndex == cbIndex) {
                            u32 constantEnd = constant.offset + constant.size;
                            calculatedSize = std::max(calculatedSize, constantEnd);
                        }
                    }

                    Msg("! [AnalyzeConstantLayout] WARNING: $Globals size=0 from Slang, calculated %u bytes from %u constants",
                        calculatedSize, extractedConstantCount);
                    implicitCB.size = calculatedSize;
                }

                if (implicitCB.size > 0) {
                    layout.constantBuffers.buffers.push_back(implicitCB);
                    Msg("  [AnalyzeConstantLayout] ✓ Added implicit $Globals CB (%u constants, %u bytes)",
                        extractedConstantCount, implicitCB.size);
                } else {
                    Msg("! [AnalyzeConstantLayout] ERROR: Cannot add $Globals CB - size is 0 even after calculation");
                }
            } else {
                Msg("  [AnalyzeConstantLayout] Skipping $Globals CB (no valid constants extracted)");
            }
        } else {
            Msg("  [AnalyzeConstantLayout] fieldCount=0, falling back to parameter iteration");

            ConstantBufferInfo implicitCB;
            implicitCB.name = "$Globals";
            implicitCB.slot = globalsBindingSlot;
            implicitCB.size = static_cast<u32>(globalParamsLayout->getSize());

            u16 cbIndex = static_cast<u16>(layout.constantBuffers.buffers.size());

            SlangInt paramCount = reflection->getParameterCount();
            Msg("  [AnalyzeConstantLayout] Scanning %d program parameters for loose uniforms", (int)paramCount);

            u32 looseConstantCount = 0;
            for (SlangInt i = 0; i < paramCount; ++i) {
                auto* param = reflection->getParameterByIndex(i);
                if (!param) continue;

                auto* typeLayout = param->getTypeLayout();
                if (!typeLayout) continue;

                auto category = typeLayout->getParameterCategory();
                if (category != slang::ParameterCategory::Uniform) {
                    continue;
                }

                const char* paramName = param->getName();
                if (!paramName) continue;

                Msg("    [AnalyzeConstantLayout] Found loose uniform: %s", paramName);

                ShaderConstant constant;
                constant.name = paramName;
                constant.offset = static_cast<u32>(param->getOffset(slang::ParameterCategory::Uniform));
                constant.cbIndex = cbIndex;

                constant.type = DetectConstantType(
                    typeLayout,
                    constant.elementSize,
                    constant.matrixStride,
                    constant.arrayCount
                );

                constant.size = (constant.arrayCount > 1)
                    ? (constant.elementSize * constant.arrayCount)
                    : static_cast<u32>(typeLayout->getSize());

                constant.frequency = InferConstantFrequency(paramName, "$Globals");
                constant.persistence = ConstantPersistence::Volatile;

                layout.constants.push_back(constant);
                looseConstantCount++;

                const char* typeStr = "unknown";
                switch (constant.type) {
                    case ShaderConstant::Type::Scalar: typeStr = "scalar"; break;
                    case ShaderConstant::Type::Vector: typeStr = "vector"; break;
                    case ShaderConstant::Type::Matrix3x4: typeStr = "float3x4"; break;
                    case ShaderConstant::Type::Matrix4x4: typeStr = "float4x4"; break;
                    case ShaderConstant::Type::Array: typeStr = "array"; break;
                    case ShaderConstant::Type::Struct: typeStr = "struct"; break;
                }

                Msg("    [AnalyzeConstantLayout] Loose constant: %s (type=%s, offset=%u, size=%u, count=%u, freq=%d)",
                    paramName, typeStr, constant.offset, constant.size, constant.arrayCount,
                    static_cast<int>(constant.frequency));
            }

            if (looseConstantCount > 0) {
                if (implicitCB.size == 0) {
                    u32 calculatedSize = 0;
                    for (const auto& constant : layout.constants) {
                        if (constant.cbIndex == cbIndex) {
                            u32 constantEnd = constant.offset + constant.size;
                            calculatedSize = std::max(calculatedSize, constantEnd);
                        }
                    }

                    Msg("! [AnalyzeConstantLayout] WARNING: $Globals size=0 from Slang (unwrapped), calculated %u bytes from %u constants",
                        calculatedSize, looseConstantCount);
                    implicitCB.size = calculatedSize;
                }

                if (implicitCB.size > 0) {
                    layout.constantBuffers.buffers.push_back(implicitCB);
                    Msg("  [AnalyzeConstantLayout] ✓ Extracted %u loose uniforms via parameter iteration (%u bytes)",
                        looseConstantCount, implicitCB.size);
                } else {
                    Msg("! [AnalyzeConstantLayout] ERROR: Cannot add $Globals CB - size is 0 even after calculation (unwrapped path)");
                }
            } else {
                Msg("  [AnalyzeConstantLayout] No loose constants found (shader uses explicit cbuffers)");
            }
        }
    }

    Msg("  [AnalyzeConstantLayout] Extracted %u total constants", layout.constants.size());

    return layout;
}

ShaderRTBindings ShaderReflector::AnalyzePixelShader(
    slang::ShaderReflection* reflection) {

    ShaderRTBindings bindings;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for pixel shader");
        return bindings;
    }

    SlangReflectionWrapper wrapper(reflection);

    auto textures = wrapper.GetTextures();
    for (const auto& tex : textures) {
        ShaderRTBindings::InputTexture input;
        input.name = tex.name;
        input.slot = tex.slot;
        bindings.inputTextures.push_back(input);
    }

    auto samplers = wrapper.GetSamplers();
    for (const auto& samp : samplers) {
        ShaderRTBindings::Sampler sampler;
        sampler.name = samp.name;
        sampler.slot = samp.slot;
        bindings.samplers.push_back(sampler);
    }

    if (reflection->getEntryPointCount() == 0) {
        Msg("! [ShaderReflector] No entry points found in pixel shader");
        return bindings;
    }

    auto* entryPoint = reflection->getEntryPointByIndex(0);
    if (!entryPoint) {
        Msg("! [ShaderReflector] Failed to get pixel shader entry point");
        return bindings;
    }

    auto* resultVarLayout = entryPoint->getResultVarLayout();
    if (resultVarLayout)
    {
        auto* typeLayout = resultVarLayout->getTypeLayout();
        if (typeLayout)
        {
            auto* type = typeLayout->getType();
            if (type && type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                u32 fieldCount = type->getFieldCount();
                Msg("  [AnalyzePixelShader] Return struct has %u fields", fieldCount);

                for (u32 i = 0; i < fieldCount; i++)
                {
                    auto* field = type->getFieldByIndex(i);
                    if (!field)
                        continue;

                    const char* fieldName = field->getName();

                    auto* fieldLayout = typeLayout->getFieldByIndex(i);
                    if (!fieldLayout)
                        continue;

                    const char* semantic = fieldLayout->getSemanticName();
                    u32 semanticIndex = fieldLayout->getSemanticIndex();

                    Msg("  [AnalyzePixelShader] Field[%u]: name='%s', semantic='%s', index=%u",
                        i, fieldName ? fieldName : "null", semantic ? semantic : "null", semanticIndex);

                    if (semantic)
                    {
                        if (xr_strcmp(semantic, "SV_Target") == 0)
                        {
                            ShaderRTBindings::OutputRT output;
                            output.slot = semanticIndex;
                            bindings.outputRTs.push_back(output);
                            Msg("  [AnalyzePixelShader] Added RT output at slot %u", semanticIndex);
                        }
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

    bindings.phase = InferPhase(bindings);

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

                    for (u32 c = 0; c < componentCount && c < 4; c++)
                    {
                        componentMask |= (1 << c);
                    }
                }

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

    result.constantLayout = AnalyzeConstantLayout(slangReflection);
    Msg("  [ShaderReflector::ExtractReflection] Extracted %u constant buffers, %u individual constants",
        result.constantLayout.constantBuffers.buffers.size(),
        result.constantLayout.constants.size());

    return result;
}

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
    return reflection ? reflection->constantLayout.constantBuffers : empty;
}

}
