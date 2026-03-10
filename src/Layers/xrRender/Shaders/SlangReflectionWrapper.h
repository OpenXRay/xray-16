#pragma once

#include <slang.h>
#include "xrCore/xrstring.h"

#include "Layers/xrRender/FrameGraph/ResourceShape.h"

namespace xray::render
{

using framegraph::ResourceShape;

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
        ResourceShape shape = ResourceShape::Texture;
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
        ResourceShape shape = ResourceShape::Unknown;
    };

public:
    SlangReflectionWrapper(slang::ShaderReflection* reflection);

    /// <summary>
    /// Get all constant buffers used by this shader
    /// </summary>
    xr_vector<CBInfo> GetConstantBuffers() const;

    /// <summary>
    /// Get all textures/SRVs used by this shader
    /// </summary>
    xr_vector<TextureInfo> GetTextures() const;

    /// <summary>
    /// Get all samplers used by this shader
    /// </summary>
    xr_vector<SamplerInfo> GetSamplers() const;

    /// <summary>
    /// Get all UAVs used by this shader
    /// </summary>
    xr_vector<UAVInfo> GetUAVs() const;

    /// <summary>
    /// Member information within a constant buffer
    /// </summary>
    struct MemberInfo
    {
        const char* name;
        u32 offset;      // Byte offset within the constant buffer
        u32 size;        // Size in bytes
        slang::TypeReflection::ScalarType scalarType;  // float, int, etc.
    };

    /// <summary>
    /// Get all members of a specific constant buffer by name
    /// Used for populating globalParams_0 from Slang-wrapped loose uniforms
    /// </summary>
    xr_vector<MemberInfo> GetConstantBufferMembers(const char* cbufferName) const;

private:
    slang::ShaderReflection* m_reflection;

    /// <summary>
    /// Helper to enumerate parameters by category
    /// </summary>
    void EnumerateParameters(
        slang::ParameterCategory category,
        std::function<void(slang::VariableLayoutReflection*)> callback) const;
};

} // namespace xray::render
