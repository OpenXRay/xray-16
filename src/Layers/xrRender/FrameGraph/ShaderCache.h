// xrRender/FrameGraph/ShaderCache.h
#pragma once

#include "xrCore/xrstring.h"

namespace xray::render::framegraph {

/// <summary>
/// Disk cache for compiled shader bytecode
/// Cache format: shaders_cache_fg/<shader_name>.<ext>/<content_hash>
///
/// Binary format:
///   [4 bytes] Cache version
///   [4 bytes] Source hash (CRC32 of shader source + macros)
///   [4 bytes] Bytecode size
///   [N bytes] Compiled bytecode
/// </summary>
class ShaderCache
{
public:
    ShaderCache();
    ~ShaderCache();

    /// <summary>
    /// Try to load shader bytecode from cache
    /// </summary>
    /// <param name="shaderName">Shader name (e.g., "gbuffer")</param>
    /// <param name="extension">Shader extension (".vs" or ".ps")</param>
    /// <param name="sourceHash">Hash of shader source code and macros</param>
    /// <param name="outBytecode">Output buffer for bytecode</param>
    /// <returns>true if cache hit, false if cache miss</returns>
    bool TryLoad(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        xr_vector<u8>& outBytecode
    );

    /// <summary>
    /// Save compiled shader bytecode to cache
    /// </summary>
    void Save(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        const xr_vector<u8>& bytecode
    );

    /// <summary>
    /// Compute hash for shader source and macros
    /// </summary>
    static u32 ComputeHash(const char* source, size_t sourceLen);
    static u32 ComputeHash(const char* source, size_t sourceLen, const char* macros);

    /// <summary>
    /// Get cache statistics
    /// </summary>
    struct Stats
    {
        u32 hits = 0;
        u32 misses = 0;
        u32 saves = 0;
    };
    const Stats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats = {}; }

private:
    /// <summary>
    /// Get cache file path for a shader
    /// </summary>
    void GetCachePath(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        string_path& outPath
    );

    static constexpr u32 CACHE_VERSION = 1;
    Stats m_stats;
};

} // namespace xray::render::framegraph
