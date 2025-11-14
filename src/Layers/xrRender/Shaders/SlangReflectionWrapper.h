#pragma once

#include <slang.h>
#include "xrCore/xrstring.h"

namespace xray::render
{

/// <summary>
/// Wrapper around Slang reflection API to extract shader resource information
/// Provides a clean interface to query constant buffers, textures, and samplers
/// </summary>
class SlangReflectionWrapper
{
public:
    /// <summary>
    /// Constant buffer information
    /// </summary>
    struct CBInfo
    {
        const char* name;
        u32 slot;        // Register number (b0, b1, b2, etc.)
        u32 space;       // Register space (for DX12/Vulkan descriptor sets)
        u32 size;        // Size in bytes
    };

    /// <summary>
    /// Texture/Structured Buffer information (SRV)
    /// </summary>
    struct TextureInfo
    {
        const char* name;
        u32 slot;        // Register number (t0, t1, t2, etc.)
        u32 space;       // Register space
    };

    /// <summary>
    /// Sampler information
    /// </summary>
    struct SamplerInfo
    {
        const char* name;
        u32 slot;        // Register number (s0, s1, s2, etc.)
        u32 space;       // Register space
    };

    /// <summary>
    /// UAV (Unordered Access View) information
    /// </summary>
    struct UAVInfo
    {
        const char* name;
        u32 slot;        // Register number (u0, u1, u2, etc.)
        u32 space;       // Register space
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
