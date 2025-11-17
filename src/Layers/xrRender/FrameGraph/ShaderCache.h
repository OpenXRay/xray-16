#pragma once

#include "xrCore/xrstring.h"
#include "ShaderReflection.h"

namespace xray::render::framegraph {

struct ExtractedReflection
{
    VertexInputSignature vertexInputSignature;
    ShaderRTBindings rtBindings;
    ShaderConstantLayout constantLayout;

    ExtractedReflection() = default;

    bool IsEmpty() const {
        return vertexInputSignature.elements.empty() &&
               constantLayout.constantBuffers.buffers.empty() &&
               rtBindings.inputTextures.empty() &&
               rtBindings.samplers.empty() &&
               rtBindings.outputRTs.empty();
    }
};

class ShaderCache
{
public:
    ShaderCache();
    ~ShaderCache();

    bool TryLoad(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        xr_vector<u8>& outBytecode,
        ExtractedReflection* outReflection = nullptr
    );

    void Save(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        const xr_vector<u8>& bytecode,
        const ExtractedReflection* reflection = nullptr
    );

    static u32 ComputeHash(const char* source, size_t sourceLen);
    static u32 ComputeHash(const char* source, size_t sourceLen, const char* macros);

    struct Stats
    {
        u32 hits = 0;
        u32 misses = 0;
        u32 saves = 0;
    };
    const Stats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats = {}; }

private:
    void GetCachePath(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        string_path& outPath
    );

    static void SerializeReflection(
        IWriter* writer,
        const ExtractedReflection& reflection
    );

    static bool DeserializeReflection(
        IReader* reader,
        ExtractedReflection& outReflection
    );

    static constexpr u32 CACHE_VERSION = 4;
    Stats m_stats;
};

}
