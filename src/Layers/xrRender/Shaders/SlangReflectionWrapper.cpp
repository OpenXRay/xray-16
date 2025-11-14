#include "stdafx.h"
#include "SlangReflectionWrapper.h"

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

    // Get the global parameters type layout
    auto* globalParamsLayout = m_reflection->getGlobalParamsTypeLayout();
    if (!globalParamsLayout)
    {
        Msg("! [SlangReflectionWrapper] No global params layout");
        return;
    }

    // Iterate through all fields in the global params structure
    unsigned int fieldCount = globalParamsLayout->getFieldCount();
    Msg("  [SlangReflectionWrapper] Enumerating category %d: found %u global fields", (int)category, fieldCount);

    for (unsigned int i = 0; i < fieldCount; ++i)
    {
        auto* fieldLayout = globalParamsLayout->getFieldByIndex(i);
        if (!fieldLayout)
            continue;

        // Check if this field matches the category we're looking for
        auto* typeLayout = fieldLayout->getTypeLayout();
        if (!typeLayout)
            continue;

        slang::ParameterCategory paramCategory = typeLayout->getParameterCategory();
        Msg("    [SlangReflectionWrapper] Field[%u]: name='%s', category=%d (looking for %d)",
            i, fieldLayout->getName(), (int)paramCategory, (int)category);

        if (paramCategory == category)
        {
            Msg("      [SlangReflectionWrapper] ✓ Match! Calling callback for '%s'", fieldLayout->getName());
            callback(fieldLayout);
        }
    }
}

} // namespace xray::render
