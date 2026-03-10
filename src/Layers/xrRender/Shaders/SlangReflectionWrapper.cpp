#include "stdafx.h"
#include "SlangReflectionWrapper.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"

namespace {

using namespace xray::render::framegraph;

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

ResourceShape ClassifyResourceShape(slang::VariableLayoutReflection* param)
{
    auto* typeLayout = param->getTypeLayout();
    if (!typeLayout) return ResourceShape::Unknown;
    auto* type = typeLayout->getType();
    if (!type) return ResourceShape::Unknown;

    auto baseShape = type->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK;
    switch (baseShape) {
    case SLANG_STRUCTURED_BUFFER:       return ResourceShape::StructuredBuffer;
    case SLANG_BYTE_ADDRESS_BUFFER:     return ResourceShape::RawBuffer;
    case SLANG_ACCELERATION_STRUCTURE:  return ResourceShape::AccelStruct;
    case SLANG_TEXTURE_1D:
    case SLANG_TEXTURE_2D:
    case SLANG_TEXTURE_3D:
    case SLANG_TEXTURE_CUBE:
    case SLANG_TEXTURE_BUFFER:          return ResourceShape::Texture;
    default: {
        auto* elementTypeLayout = typeLayout->getElementTypeLayout();
        if (elementTypeLayout) {
            auto* elementType = elementTypeLayout->getType();
            if (elementType) {
                auto innerShape = elementType->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK;
                switch (innerShape) {
                case SLANG_STRUCTURED_BUFFER:       return ResourceShape::StructuredBuffer;
                case SLANG_BYTE_ADDRESS_BUFFER:     return ResourceShape::RawBuffer;
                case SLANG_ACCELERATION_STRUCTURE:  return ResourceShape::AccelStruct;
                case SLANG_TEXTURE_1D:
                case SLANG_TEXTURE_2D:
                case SLANG_TEXTURE_3D:
                case SLANG_TEXTURE_CUBE:
                case SLANG_TEXTURE_BUFFER:          return ResourceShape::Texture;
                default: break;
                }
            }
        }
        auto kind = type->getKind();
        if (kind == slang::TypeReflection::Kind::Resource) {
            Msg("! [ClassifyResourceShape] Unknown resource shape 0x%x for '%s' (kind=Resource)",
                type->getResourceShape(), param->getName() ? param->getName() : "?");
        }
        return ResourceShape::Unknown;
    }
    }
}

} // anonymous namespace

namespace xray::render
{

SlangReflectionWrapper::SlangReflectionWrapper(slang::ShaderReflection* reflection)
    : m_reflection(reflection)
{
    VERIFY(reflection);
}

xr_vector<SlangReflectionWrapper::CBInfo> SlangReflectionWrapper::GetConstantBuffers() const
{
    xr_vector<CBInfo> result;

    EnumerateParameters(slang::ParameterCategory::ConstantBuffer, [&](slang::VariableLayoutReflection* param)
    {
        CBInfo info = {};
        info.name = param->getName();
        info.slot = static_cast<u32>(param->getOffset(slang::ParameterCategory::ConstantBuffer));
        info.space = static_cast<u32>(param->getBindingSpace(slang::ParameterCategory::ConstantBuffer));

        // Get the size of the constant buffer in bytes
        // The constant buffer is a struct, so we need to get the element type's size in Uniform category
        auto* typeLayout = param->getTypeLayout();
        if (typeLayout)
        {
            // Get the underlying element type (the struct inside the cbuffer)
            auto* elementTypeLayout = typeLayout->getElementTypeLayout();
            if (elementTypeLayout)
            {
                info.size = static_cast<u32>(elementTypeLayout->getSize(slang::ParameterCategory::Uniform));
            }
            else
            {
                // Fallback: try getting size directly
                info.size = static_cast<u32>(typeLayout->getSize(slang::ParameterCategory::Uniform));
            }
        }

        result.push_back(info);
    });

    return result;
}

xr_vector<SlangReflectionWrapper::TextureInfo> SlangReflectionWrapper::GetTextures() const
{
    xr_vector<TextureInfo> result;

    EnumerateParameters(slang::ParameterCategory::ShaderResource, [&](slang::VariableLayoutReflection* param)
    {
        TextureInfo info = {};
        info.name = param->getName();
        info.slot = static_cast<u32>(param->getOffset(slang::ParameterCategory::ShaderResource));
        info.space = static_cast<u32>(param->getBindingSpace(slang::ParameterCategory::ShaderResource));
        info.shape = ClassifyResourceShape(param);

        result.push_back(info);
    });

    return result;
}

xr_vector<SlangReflectionWrapper::SamplerInfo> SlangReflectionWrapper::GetSamplers() const
{
    xr_vector<SamplerInfo> result;

    EnumerateParameters(slang::ParameterCategory::SamplerState, [&](slang::VariableLayoutReflection* param)
    {
        SamplerInfo info = {};
        info.name = param->getName();
        info.slot = static_cast<u32>(param->getOffset(slang::ParameterCategory::SamplerState));
        info.space = static_cast<u32>(param->getBindingSpace(slang::ParameterCategory::SamplerState));

        result.push_back(info);
    });

    return result;
}

xr_vector<SlangReflectionWrapper::UAVInfo> SlangReflectionWrapper::GetUAVs() const
{
    xr_vector<UAVInfo> result;

    EnumerateParameters(slang::ParameterCategory::UnorderedAccess, [&](slang::VariableLayoutReflection* param)
    {
        UAVInfo info = {};
        info.name = param->getName();
        info.slot = static_cast<u32>(param->getOffset(slang::ParameterCategory::UnorderedAccess));
        info.space = static_cast<u32>(param->getBindingSpace(slang::ParameterCategory::UnorderedAccess));
        info.shape = ClassifyResourceShape(param);

        result.push_back(info);
    });

    return result;
}

xr_vector<SlangReflectionWrapper::MemberInfo> SlangReflectionWrapper::GetConstantBufferMembers(const char* cbufferName) const
{
    xr_vector<MemberInfo> members;

    if (!m_reflection || !cbufferName)
        return members;

    // Find the constant buffer by name
    EnumerateParameters(slang::ParameterCategory::ConstantBuffer, [&](slang::VariableLayoutReflection* param)
    {
        if (xr_strcmp(param->getName(), cbufferName) == 0)
        {
            // Found the constant buffer! Now enumerate its fields
            auto* typeLayout = param->getTypeLayout();
            if (!typeLayout)
                return;

            auto* type = typeLayout->getType();
            if (!type)
                return;

            u32 fieldCount = type->getFieldCount();
            for (u32 f = 0; f < fieldCount; ++f)
            {
                auto* field = type->getFieldByIndex(f);
                auto* fieldLayout = typeLayout->getFieldByIndex(f);

                if (!field || !fieldLayout)
                    continue;

                MemberInfo info = {};
                info.name = field->getName();
                info.offset = static_cast<u32>(fieldLayout->getOffset(slang::ParameterCategory::Uniform));

                // Get size from the type layout (VariableLayoutReflection -> TypeLayoutReflection -> getSize())
                auto* fieldTypeLayout = fieldLayout->getTypeLayout();
                info.size = fieldTypeLayout ? static_cast<u32>(fieldTypeLayout->getSize()) : 0;

                auto* fieldType = field->getType();
                if (fieldType)
                {
                    info.scalarType = fieldType->getScalarType();
                }

                members.push_back(info);
            }
        }
    });

    return members;
}

void SlangReflectionWrapper::EnumerateParameters(
    slang::ParameterCategory category,
    std::function<void(slang::VariableLayoutReflection*)> callback) const
{
    if (!m_reflection)
        return;

    // CORRECT APPROACH: Iterate through shader-level parameters (NOT type layout fields!)
    // Slang stores constant buffers, textures, etc. as shader parameters
    unsigned int shaderParamCount = m_reflection->getParameterCount();

    for (unsigned int i = 0; i < shaderParamCount; ++i)
    {
        auto* param = m_reflection->getParameterByIndex(i);
        if (!param)
            continue;

        // Check if this parameter matches the category we're looking for
        auto* typeLayout = param->getTypeLayout();
        if (!typeLayout)
            continue;

        slang::ParameterCategory paramCategory = typeLayout->getParameterCategory();

        if (paramCategory == category)
        {
            callback(param);
        }
    }
}

} // namespace xray::render
