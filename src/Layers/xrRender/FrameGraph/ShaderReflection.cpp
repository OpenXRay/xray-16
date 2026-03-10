// xrRender/FrameGraph/ShaderReflection.cpp
#include "stdafx.h"
#include "ShaderReflection.h"
#include "ShaderCache.h"  // For ExtractedReflection definition
#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include <slang.h>

namespace xray::render::framegraph {

namespace {

// ═══════════════════════════════════════════════════
// Type Detection Helper for Constant Reflection
// ═══════════════════════════════════════════════════

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

            // Query stride (accounts for HLSL packing rules - always 16 bytes per row)
            outMatrixStride = static_cast<u16>(typeLayout->getStride(slang::ParameterCategory::Uniform));
            outElementSize = static_cast<u16>(typeLayout->getSize());
            outArrayCount = 1;

            if (rowCount == 3 && colCount == 4) {
                // float3x4: 3 rows × 4 columns = 48 bytes (stride 16 per row)
                return ShaderConstant::Type::Matrix3x4;
            } else if (rowCount == 4 && colCount == 4) {
                // float4x4: 4 rows × 4 columns = 64 bytes (stride 16 per row)
                return ShaderConstant::Type::Matrix4x4;
            }

            Msg("! [DetectConstantType] Unsupported matrix dimensions: %dx%d",
                rowCount, colCount);
            return ShaderConstant::Type::Struct;
        }

        case slang::TypeReflection::Kind::Array: {
            // Get array element type and count
            auto* elementTypeLayout = typeLayout->getElementTypeLayout();

            outArrayCount = static_cast<u16>(typeLayout->getElementCount());
            outElementSize = elementTypeLayout ? static_cast<u16>(elementTypeLayout->getSize()) : 0;
            outMatrixStride = 0;

            // Check if array of matrices
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

} // anonymous namespace

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

    slang::VariableLayoutReflection* inputParam = nullptr;
    for (u32 i = 0; i < paramCount; i++) {
        auto* param = entryPoint->getParameterByIndex(i);
        if (!param) continue;

        auto* typeLayout = param->getTypeLayout();
        if (!typeLayout) continue;

        if (typeLayout->getParameterCategory() == slang::ParameterCategory::VaryingInput) {
            inputParam = param;
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

    for (u32 i = 0; i < fieldCount; i++) {
        auto* field = inputType->getFieldByIndex(i);
        auto* fieldLayout = inputTypeLayout->getFieldByIndex(i);

        if (!field || !fieldLayout)
            continue;

        const char* fieldName = field->getName() ? field->getName() : "<unknown>";

        // Get semantic name (POSITION, TEXCOORD, etc.)
        const char* semanticName = fieldLayout->getSemanticName();

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
        elem.inputSlot = 0;

        signature.elements.push_back(elem);
    }

    Msg("  [ShaderReflector] VS input before sort (%u elements):", signature.elements.size());
    for (u32 i = 0; i < signature.elements.size(); i++) {
        const auto& e = signature.elements[i];
        Msg("    [%u] semantic='%s' index=%u format=%d", i, e.semanticName.c_str(), e.semanticIndex, (int)e.format);
    }

    auto semanticPriority = [](const char* name, u32 idx) -> int {
        if (xr_strcmp(name, "POSITION") == 0 || xr_strcmp(name, "POSITIONT") == 0) return 0;
        if (xr_strcmp(name, "NORMAL") == 0) return 100;
        if (xr_strcmp(name, "TANGENT") == 0) return 200;
        if (xr_strcmp(name, "BINORMAL") == 0) return 300;
        if (xr_strcmp(name, "TEXCOORD") == 0) return 400 + idx;
        if (xr_strcmp(name, "COLOR") == 0) return 500 + idx;
        return 1000;
    };
    std::sort(signature.elements.begin(), signature.elements.end(),
        [&](const auto& a, const auto& b) {
            return semanticPriority(a.semanticName.c_str(), a.semanticIndex) <
                   semanticPriority(b.semanticName.c_str(), b.semanticIndex);
        });

    Msg("  [ShaderReflector] VS input after sort:");
    for (u32 i = 0; i < signature.elements.size(); i++) {
        const auto& e = signature.elements[i];
        Msg("    [%u] semantic='%s' index=%u format=%d", i, e.semanticName.c_str(), e.semanticIndex, (int)e.format);
    }

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
//  INFER CONSTANT UPDATE FREQUENCY
// ═══════════════════════════════════════════════════

UpdateFrequency ShaderReflector::InferConstantFrequency(const char* name, const char* cbName) {
    // ═══════════════════════════════════════════════════
    //  FREQUENCY CLASSIFICATION RULES
    // ═══════════════════════════════════════════════════
    // IMPORTANT: Check constant names BEFORE CB names to allow per-constant overrides!

    // Engine-frequency (samplers, global state)
    if (strstr(cbName, "static_globals") != nullptr) {
        return UpdateFrequency::Engine;
    }

    // Pass-frequency (view/projection matrices, camera state)
    if (strstr(name, "m_VP") != nullptr ||
        strstr(name, "m_V") != nullptr ||
        strstr(name, "m_P") != nullptr ||
        strstr(name, "eye_position") != nullptr ||
        strstr(name, "eye_direction") != nullptr) {
        return UpdateFrequency::Pass;
    }

    // Instance-frequency (world matrix, bones) - CHECK BEFORE CB NAME!
    // This allows m_W/m_WV/m_WVP to be Instance even if in dynamic_transforms CB
    if (strstr(name, "m_World") != nullptr ||
        strstr(name, "m_W") != nullptr ||  // Matches m_W, m_WV, m_WVP
        strstr(name, "m_xform") != nullptr ||
        strstr(name, "bones") != nullptr ||
        strstr(cbName, "$Globals") != nullptr ||
        strstr(cbName, "SkeletonBones") != nullptr) {
        return UpdateFrequency::Instance;
    }

    // Material-frequency (textures, detail scale, material params)
    if (strstr(name, "dt_params") != nullptr ||
        strstr(name, "material") != nullptr ||
        strstr(cbName, "dynamic_transforms") != nullptr) {
        return UpdateFrequency::Material;
    }

    // Default to Instance if unclear
    return UpdateFrequency::Instance;
}

// ═══════════════════════════════════════════════════
//  INFER CONSTANT PERSISTENCE (STATIC VS VOLATILE)
// ═══════════════════════════════════════════════════
//
// Determines whether a constant should use static (persistent) or volatile (VCB) allocation.
// Key heuristic: If CB name matches MaterialPSO's constantBuffers (not vcbRequirements),
// it's a static CB that exists persistently rather than volatile per-frame.
//
ConstantPersistence ShaderReflector::InferConstantPersistence(const char* cbName, UpdateFrequency frequency) {
    // Determines whether a CB should be:
    // - Static: Persistent shared buffer (created once, updated as needed, shared across draws)
    // - Volatile: VCB ring-buffered (new instance per draw from pool)

    // Static CBs (persistent shared buffers)
    if (strstr(cbName, "static_globals") != nullptr ||
        strstr(cbName, "dynamic_transforms") != nullptr) {
        return ConstantPersistence::Static;  // Shared global buffer
    }

    // Volatile CBs (VCB ring-buffered - per-draw instances)
    if (strstr(cbName, "$Globals") != nullptr ||
        strstr(cbName, "SkeletonBones") != nullptr ||
        strstr(cbName, "GlobalParams") != nullptr ||  // Slang-generated CB containing sbones_array
        strstr(cbName, "PerInstanceTransforms") != nullptr ||
        strstr(cbName, "TreeData") != nullptr ||
        strstr(cbName, "GrassWind") != nullptr) {
        return ConstantPersistence::Volatile;  // VCB pool
    }

    // Default behavior based on frequency:
    // - Engine/Pass frequency = static (shared across draws)
    // - Instance/Material = volatile (per-draw data)
    if (frequency == UpdateFrequency::Engine || frequency == UpdateFrequency::Pass) {
        return ConstantPersistence::Static;
    }

    return ConstantPersistence::Volatile;
}

// ═══════════════════════════════════════════════════
//  ANALYZE CONSTANT LAYOUT (CB + PER-CONSTANT METADATA)
// ═══════════════════════════════════════════════════

ShaderConstantLayout ShaderReflector::AnalyzeConstantLayout(slang::ShaderReflection* reflection) {
    ShaderConstantLayout layout;

    if (!reflection) {
        Msg("! [ShaderReflector] Null Slang reflection for constant layout analysis");
        return layout;
    }

    // First, get CB-level metadata (existing)
    layout.constantBuffers = AnalyzeConstantBuffers(reflection);

    // Now extract per-constant metadata from each CB
    SlangReflectionWrapper wrapper(reflection);
    auto constantBuffers = wrapper.GetConstantBuffers();

    for (u32 cbIdx = 0; cbIdx < constantBuffers.size(); ++cbIdx) {
        const auto& cbInfo = constantBuffers[cbIdx];

        // Get parameter count for this CB
        u32 paramCount = reflection->getParameterCount();

        bool foundMatchingParam = false;
        for (u32 paramIdx = 0; paramIdx < paramCount; ++paramIdx) {
            auto* param = reflection->getParameterByIndex(paramIdx);
            if (!param) {
                continue;
            }

            const char* paramName = param->getName();

            auto* typeLayout = param->getTypeLayout();
            if (!typeLayout) {
                continue;
            }

            // Check if this parameter is a constant buffer
            auto paramCategory = typeLayout->getParameterCategory();
            if (paramCategory != slang::ParameterCategory::ConstantBuffer) {
                continue;
            }

            // Get the CB name
            if (!paramName || xr_strcmp(paramName, cbInfo.name) != 0) {
                continue;
            }

            foundMatchingParam = true;

            // UNWRAP the constant buffer to get the struct inside
            // typeLayout is ConstantBuffer<StructType>, we need to get StructType
            auto* elementTypeLayout = typeLayout->getElementTypeLayout();
            if (!elementTypeLayout) {
                Msg("! [AnalyzeConstantLayout] ERROR: No elementTypeLayout (CB wrapper is empty?)");
                continue;
            }

            auto* cbType = elementTypeLayout->getType();
            if (!cbType) {
                Msg("! [AnalyzeConstantLayout] ERROR: No cbType from element");
                continue;
            }

            auto typeKind = cbType->getKind();

            if (typeKind != slang::TypeReflection::Kind::Struct) {
                Msg("! [AnalyzeConstantLayout] ERROR: Not a struct type after unwrapping");
                continue;
            }

            // Enumerate fields (individual constants)
            u32 fieldCount = cbType->getFieldCount();

            // Use elementTypeLayout for field access, not typeLayout
            for (u32 i = 0; i < fieldCount; ++i) {
                auto* field = cbType->getFieldByIndex(i);
                auto* fieldLayout = elementTypeLayout->getFieldByIndex(i);  // Use elementTypeLayout!

                if (!field || !fieldLayout) continue;

                const char* fieldName = field->getName();
                if (!fieldName) continue;

                ShaderConstant constant;
                constant.name = fieldName;
                constant.offset = (u32)fieldLayout->getOffset(slang::ParameterCategory::Uniform);

                // NEW: Detect type using helper function
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

                // Infer frequency from name/CB
                constant.frequency = InferConstantFrequency(fieldName, cbInfo.name);

                // Infer persistence (static vs volatile) based on CB name
                constant.persistence = InferConstantPersistence(cbInfo.name, constant.frequency);

                layout.constants.push_back(constant);
            }
        }

        if (!foundMatchingParam) {
            Msg("  [AnalyzeConstantLayout]   WARNING: No matching parameter found for CB '%s'", cbInfo.name);
        }
    }

    // ═══════════════════════════════════════════════════
    // STEP 2: Extract Implicit $Globals (Loose Constants) ⚠️ CRITICAL FIX
    // ═══════════════════════════════════════════════════

    auto* globalParamsLayout = reflection->getGlobalParamsTypeLayout();

    if (!globalParamsLayout) {
        // No loose constants in shader
    } else {
        // Query binding slot from Slang BEFORE branching (both paths need this!)
        auto* globalParamsVar = reflection->getGlobalParamsVarLayout();
        u32 globalsBindingSlot = 0;

        if (globalParamsVar) {
            globalsBindingSlot = static_cast<u32>(globalParamsVar->getOffset(slang::ParameterCategory::ConstantBuffer));
        } else {
            // Fallback: $Globals typically at b0 (per-object constants)
            globalsBindingSlot = 0;
        }

        // CRITICAL FIX: Slang sometimes wraps loose uniforms into a struct, sometimes doesn't
        // Try getFieldCount() first (wrapped case), fallback to parameter iteration (unwrapped case)
        u32 fieldCount = static_cast<u32>(globalParamsLayout->getFieldCount());

        if (fieldCount > 0) {
            // Create implicit $Globals constant buffer entry (but don't add it yet)
            ConstantBufferInfo implicitCB;
            implicitCB.name = "$Globals";
            implicitCB.slot = globalsBindingSlot;  // Use pre-queried binding
            implicitCB.size = static_cast<u32>(globalParamsLayout->getSize());

            u16 cbIndex = static_cast<u16>(layout.constantBuffers.buffers.size());
            u32 extractedConstantCount = 0;

            // Extract individual loose constants using field API
            for (u32 i = 0; i < fieldCount; ++i) {
                auto* fieldLayout = globalParamsLayout->getFieldByIndex(i);
                if (!fieldLayout) {
                    continue;
                }

                // CRITICAL: Skip non-uniform resources (textures, samplers, CB references)
                // Only process actual uniform constants (category=8)
                auto* fieldTypeLayout = fieldLayout->getTypeLayout();
                if (fieldTypeLayout) {
                    auto category = fieldTypeLayout->getParameterCategory();
                    if (category != slang::ParameterCategory::Uniform) {
                        // Skip this field - it's a texture, sampler, or CB reference, not a loose constant
                        continue;
                    }
                }

                // Get field name from variable reflection
                auto* fieldVar = fieldLayout->getVariable();
                if (!fieldVar) {
                    continue;
                }

                const char* fieldName = fieldVar->getName();
                if (!fieldName) {
                    continue;
                }

                ShaderConstant constant;
                constant.name = fieldName;
                constant.offset = static_cast<u32>(fieldLayout->getOffset(slang::ParameterCategory::Uniform));
                constant.cbIndex = cbIndex;

                // Detect type (scalar/vector/matrix/array) using the helper
                // (fieldTypeLayout already obtained above for category check)
                constant.type = DetectConstantType(
                    fieldTypeLayout,
                    constant.elementSize,
                    constant.matrixStride,
                    constant.arrayCount
                );

                constant.size = (constant.arrayCount > 1)
                    ? (constant.elementSize * constant.arrayCount)
                    : static_cast<u32>(fieldTypeLayout ? fieldTypeLayout->getSize() : 0);

                // Infer frequency and persistence
                constant.frequency = InferConstantFrequency(fieldName, "$Globals");
                constant.persistence = ConstantPersistence::Volatile;  // Loose constants are volatile

                layout.constants.push_back(constant);
                extractedConstantCount++;
            }

            // Only add the $Globals CB if we extracted valid constants
            if (extractedConstantCount > 0) {
                // Sanity check: if we have constants, size should be > 0
                if (implicitCB.size == 0) {
                    // Slang reflection bug: calculate size from extracted constants
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
                } else {
                    Msg("! [AnalyzeConstantLayout] ERROR: Cannot add $Globals CB - size is 0 even after calculation");
                }
            }
        } else {
            // FALLBACK: fieldCount == 0 means Slang didn't wrap loose uniforms into a struct
            // Iterate program parameters directly and extract category=Uniform (8)

            // Create implicit $Globals CB anyway
            ConstantBufferInfo implicitCB;
            implicitCB.name = "$Globals";
            implicitCB.slot = globalsBindingSlot;  // Use pre-queried binding (same as wrapped path!)
            implicitCB.size = static_cast<u32>(globalParamsLayout->getSize());

            u16 cbIndex = static_cast<u16>(layout.constantBuffers.buffers.size());

            // Iterate all program parameters to find loose uniforms (category=8)
            SlangInt paramCount = reflection->getParameterCount();

            u32 looseConstantCount = 0;
            for (SlangInt i = 0; i < paramCount; ++i) {
                auto* param = reflection->getParameterByIndex(i);
                if (!param) continue;

                auto* typeLayout = param->getTypeLayout();
                if (!typeLayout) continue;

                // Check if this is a loose uniform (category=8)
                auto category = typeLayout->getParameterCategory();
                if (category != slang::ParameterCategory::Uniform) {
                    continue;  // Not a loose uniform
                }

                const char* paramName = param->getName();
                if (!paramName) continue;

                ShaderConstant constant;
                constant.name = paramName;
                constant.offset = static_cast<u32>(param->getOffset(slang::ParameterCategory::Uniform));
                constant.cbIndex = cbIndex;

                // Detect type
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
            }

            if (looseConstantCount > 0) {
                // Sanity check: calculate size from constants if Slang returned 0
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
                } else {
                    Msg("! [AnalyzeConstantLayout] ERROR: Cannot add $Globals CB - size is 0 even after calculation (unwrapped path)");
                }
            }
        }
    }

    return layout;
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

    auto textures = wrapper.GetTextures();
    for (const auto& tex : textures) {
        if (tex.space != 0) continue;
        bindings.inputTextures.emplace_back(tex.name, tex.slot, tex.shape);
    }

    auto samplers = wrapper.GetSamplers();
    for (const auto& samp : samplers) {
        ShaderRTBindings::Sampler sampler;
        sampler.name = samp.name;
        sampler.slot = samp.slot;
        bindings.samplers.push_back(sampler);
    }

    auto uavs = wrapper.GetUAVs();
    for (const auto& uav : uavs) {
        if (uav.space != 0) continue;
        bindings.uavBindings.emplace_back(uav.name, uav.slot, uav.shape);
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

                    if (semantic)
                    {
                        // Check for SV_Target outputs
                        if (xr_strcmp(semantic, "SV_Target") == 0)
                        {
                            ShaderRTBindings::OutputRT output;
                            output.slot = semanticIndex;
                            bindings.outputRTs.push_back(output);
                        }
                        // Check for depth output
                        else if (xr_strcmp(semantic, "SV_Depth") == 0)
                        {
                            bindings.hasDepthOutput = true;
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

                if (semantic && xr_strcmp(semantic, "SV_Target") == 0)
                {
                    ShaderRTBindings::OutputRT output;
                    output.slot = semanticIndex;
                    bindings.outputRTs.push_back(output);
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

    // Extract all reflection data using existing Analyze methods
    if (isVertexShader)
    {
        result.vertexInputSignature = AnalyzeVertexShader(slangReflection);
    }
    else
    {
        result.rtBindings = AnalyzePixelShader(slangReflection);
    }

    // Extract full constant layout (CB + per-constant metadata)
    // This includes implicit $Globals CB for loose uniforms
    result.constantLayout = AnalyzeConstantLayout(slangReflection);

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
    return reflection ? reflection->constantLayout.constantBuffers : empty;
}

} // namespace xray::render::framegraph
