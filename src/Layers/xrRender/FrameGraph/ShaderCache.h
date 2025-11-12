#pragma once

#include "xrCore/xrstring.h"

namespace xray::render::framegraph {

class ShaderCache
{
public:
    ShaderCache();
    ~ShaderCache();

    bool TryLoad(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        xr_vector<u8>& outBytecode
    );

    void Save(
        const char* shaderName,
        const char* extension,
        u32 sourceHash,
        const xr_vector<u8>& bytecode
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

    static constexpr u32 CACHE_VERSION = 1;
    Stats m_stats;
};

}
