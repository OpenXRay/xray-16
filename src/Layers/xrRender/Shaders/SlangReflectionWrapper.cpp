#include "stdafx.h"
#include "SlangReflectionWrapper.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"

namespace {

using namespace xray::render::framegraph;

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

        auto* typeLayout = param->getTypeLayout();
        if (typeLayout)
        {
            auto* elementTypeLayout = typeLayout->getElementTypeLayout();
            if (elementTypeLayout)
            {
                info.size = static_cast<u32>(elementTypeLayout->getSize(slang::ParameterCategory::Uniform));
            }
            else
            {
                info.size = static_cast<u32>(typeLayout->getSize(slang::ParameterCategory::Uniform));
            }
        }

        Msg("      [GetConstantBuffers] Extracted '%s' -> slot=%u, space=%u, size=%u", info.name, info.slot, info.space, info.size);
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

        Msg("      [GetTextures] Extracted '%s' -> slot=%u, space=%u", info.name, info.slot, info.space);
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

        Msg("      [GetSamplers] Extracted '%s' -> slot=%u, space=%u", info.name, info.slot, info.space);
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

        result.push_back(info);
    });

    return result;
}

xr_vector<SlangReflectionWrapper::MemberInfo> SlangReflectionWrapper::GetConstantBufferMembers(const char* cbufferName) const
{
    xr_vector<MemberInfo> members;

    if (!m_reflection || !cbufferName)
        return members;

    EnumerateParameters(slang::ParameterCategory::ConstantBuffer, [&](slang::VariableLayoutReflection* param)
    {
        if (xr_strcmp(param->getName(), cbufferName) == 0)
        {
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

    Msg("  [SlangReflectionWrapper::EnumerateParameters] === DEBUGGING REFLECTION STRUCTURE ===");

    unsigned int shaderParamCount = m_reflection->getParameterCount();
    Msg("  [SlangReflectionWrapper] Shader has %u parameters (via getParameterCount())", shaderParamCount);

    for (unsigned int i = 0; i < shaderParamCount; ++i)
    {
        auto* param = m_reflection->getParameterByIndex(i);
        if (param)
        {
            const char* paramName = param->getName();
            auto* paramTypeLayout = param->getTypeLayout();
            if (paramTypeLayout)
            {
                slang::ParameterCategory paramCat = paramTypeLayout->getParameterCategory();
                Msg("    [SlangReflectionWrapper] Shader Param[%u]: name='%s', category=%d",
                    i, paramName ? paramName : "NULL", (int)paramCat);
            }
        }
    }

    unsigned int entryPointCount = m_reflection->getEntryPointCount();
    Msg("  [SlangReflectionWrapper] Total entry points: %u", entryPointCount);

    for (unsigned int epIdx = 0; epIdx < entryPointCount; ++epIdx)
    {
        auto* entryPoint = m_reflection->getEntryPointByIndex(epIdx);
        if (entryPoint)
        {
            Msg("    [SlangReflectionWrapper] EntryPoint[%u]: name='%s'", epIdx, entryPoint->getName());

            auto* entryParamsLayout = entryPoint->getTypeLayout();
            if (entryParamsLayout)
            {
                unsigned int entryFieldCount = entryParamsLayout->getFieldCount();
                Msg("      [SlangReflectionWrapper] Entry point has %u fields", entryFieldCount);

                for (unsigned int i = 0; i < entryFieldCount; ++i)
                {
                    auto* fieldLayout = entryParamsLayout->getFieldByIndex(i);
                    if (fieldLayout)
                    {
                        auto* typeLayout = fieldLayout->getTypeLayout();
                        if (typeLayout)
                        {
                            slang::ParameterCategory paramCat = typeLayout->getParameterCategory();
                            Msg("        [SlangReflectionWrapper] EntryField[%u]: name='%s', category=%d",
                                i, fieldLayout->getName(), (int)paramCat);
                        }
                    }
                }
            }
        }
    }

    Msg("  [SlangReflectionWrapper] Enumerating shader parameters (looking for category %d)", (int)category);

    for (unsigned int i = 0; i < shaderParamCount; ++i)
    {
        auto* param = m_reflection->getParameterByIndex(i);
        if (!param)
            continue;

        auto* typeLayout = param->getTypeLayout();
        if (!typeLayout)
            continue;

        slang::ParameterCategory paramCategory = typeLayout->getParameterCategory();

        if (paramCategory == category)
        {
            Msg("      [SlangReflectionWrapper] ✓ Match! Param[%u]: name='%s', category=%d",
                i, param->getName(), (int)paramCategory);
            callback(param);
        }
    }
}

}
