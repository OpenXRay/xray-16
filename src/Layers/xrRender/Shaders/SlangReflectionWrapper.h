#pragma once

#include <slang.h>
#include "xrCore/xrstring.h"

namespace xray::render
{

class SlangReflectionWrapper
{
public:
    struct CBInfo
    {
        const char* name;
        u32 slot;
        u32 space;
        u32 size;
    };

    struct TextureInfo
    {
        const char* name;
        u32 slot;
        u32 space;
    };

    struct SamplerInfo
    {
        const char* name;
        u32 slot;
        u32 space;
    };

    struct UAVInfo
    {
        const char* name;
        u32 slot;
        u32 space;
    };

public:
    SlangReflectionWrapper(slang::ShaderReflection* reflection);

    xr_vector<CBInfo> GetConstantBuffers() const;

    xr_vector<TextureInfo> GetTextures() const;

    xr_vector<SamplerInfo> GetSamplers() const;

    xr_vector<UAVInfo> GetUAVs() const;

    struct MemberInfo
    {
        const char* name;
        u32 offset;
        u32 size;
        slang::TypeReflection::ScalarType scalarType;
    };

    xr_vector<MemberInfo> GetConstantBufferMembers(const char* cbufferName) const;

private:
    slang::ShaderReflection* m_reflection;

    void EnumerateParameters(
        slang::ParameterCategory category,
        std::function<void(slang::VariableLayoutReflection*)> callback) const;
};

}
