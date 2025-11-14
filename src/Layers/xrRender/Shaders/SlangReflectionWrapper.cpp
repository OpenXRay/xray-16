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
        auto* typeLayout = param->getTypeLayout();
        if (typeLayout)
        {
            info.size = static_cast<u32>(typeLayout->getSize(slang::ParameterCategory::Uniform));
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

        result.push_back(info);
    });

    return result;
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
        return;

    // Iterate through all fields in the global params structure
    unsigned int fieldCount = globalParamsLayout->getFieldCount();
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
        if (paramCategory == category)
        {
            callback(fieldLayout);
        }
    }
}

} // namespace xray::render
