// xrRender/FrameGraph/ShaderCache.h
#pragma once

#include "xrCore/xrstring.h"
#include "ShaderReflection.h"  // For VertexInputSignature, ShaderConstantBuffers, ShaderRTBindings

namespace xray::render::framegraph {

/// <summary>
/// Extracted reflection data from shader compilation
/// This is the serializable snapshot of Slang reflection
/// </summary>
struct ExtractedReflection
{
    VertexInputSignature vertexInputSignature;
    ShaderConstantBuffers constantBuffers;
    ShaderRTBindings rtBindings;
    ShaderConstantLayout constantLayout;  // NEW: Full layout with per-constant data

    ExtractedReflection() = default;

    /// Check if this reflection has any data
    bool IsEmpty() const {
        return vertexInputSignature.elements.empty() &&
               constantBuffers.buffers.empty() &&
               rtBindings.inputTextures.empty() &&
               rtBindings.samplers.empty() &&
               rtBindings.outputRTs.empty();
    }
};

/// <summary>
/// Disk cache for compiled shader bytecode + reflection metadata
/// Cache format: shaders_cache_fg/<shader_name>.<ext>/<content_hash>
///
/// Binary format (CACHE_VERSION = 2):
///   [4 bytes] Cache version
///   [4 bytes] Source hash (CRC32 of shader source + macros)
///   [4 bytes] Bytecode size
///   [N bytes] Compiled bytecode
///   [1 byte]  Has reflection flag (0 = no reflection, 1 = has reflection)
///   IF has reflection:
///     [4 bytes] Reflection blob size
///     [M bytes] Reflection blob (serialized ExtractedReflection)
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
    /// <param name="outReflection">Optional: Output buffer for reflection (nullptr = don't load reflection)</param>
    /// <returns>true if cache hit, false if cache miss</returns>
    bool TryLoad(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        xr_vector<u8>& outBytecode,
        ExtractedReflection* outReflection = nullptr
    );

    /// <summary>
    /// Save compiled shader bytecode to cache
    /// </summary>
    /// <param name="reflection">Optional: Reflection data to cache (nullptr = bytecode only)</param>
    void Save(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        const xr_vector<u8>& bytecode,
        const ExtractedReflection* reflection = nullptr
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

    /// <summary>
    /// Serialize reflection data to binary blob
    /// </summary>
    static void SerializeReflection(
        IWriter* writer,
        const ExtractedReflection& reflection
    );

    /// <summary>
    /// Deserialize reflection data from binary blob
    /// </summary>
    static bool DeserializeReflection(
        IReader* reader,
        ExtractedReflection& outReflection
    );

    static constexpr u32 CACHE_VERSION = 3;  // Bumped for per-constant metadata (constantLayout.constants)
    Stats m_stats;
};

} // namespace xray::render::framegraph
