// xrRender/FrameGraph/ShaderCache.cpp
#include "stdafx.h"
#include "ShaderCache.h"
#include "xrCore/FileCRC32.h"

namespace xray::render::framegraph {

ShaderCache::ShaderCache()
{
    Msg("* [ShaderCache] Initialized (cache dir: shaders_cache_fg/)");
}

ShaderCache::~ShaderCache()
{
    Msg("* [ShaderCache] Stats - Hits: %u, Misses: %u, Saves: %u",
        m_stats.hits, m_stats.misses, m_stats.saves);
}

void ShaderCache::GetCachePath(
    const char* shaderName,
    const char* extension,
    u32 sourceHash,
    string_path& outPath)
{
    // Format: shaders_cache_fg/<shader_name>.<ext>/<source_hash>
    // Example: shaders_cache_fg/gbuffer.vs/A3F2B1C0
    string_path shaderDir;
    xr_sprintf(shaderDir, "shaders_cache_fg%s%s%s",
        DELIMITER, shaderName, extension);

    xr_sprintf(outPath, "%s%s%08X",
        shaderDir, DELIMITER, sourceHash);
}

bool ShaderCache::TryLoad(
    const char* shaderName,
    const char* extension,
    u32 sourceHash,
    xr_vector<u8>& outBytecode)
{
    string_path cachePath;
    GetCachePath(shaderName, extension, sourceHash, cachePath);

    // Check if cache file exists
    if (!FS.exist("$app_data_root$", cachePath))
    {
        m_stats.misses++;
        return false;
    }

    // Open cache file
    IReader* reader = FS.r_open("$app_data_root$", cachePath);
    if (!reader)
    {
        m_stats.misses++;
        Msg("! [ShaderCache] Failed to open cache file: %s", cachePath);
        return false;
    }

    // Read header
    u32 version = reader->r_u32();
    u32 storedHash = reader->r_u32();
    u32 bytecodeSize = reader->r_u32();

    // Validate version and hash
    if (version != CACHE_VERSION)
    {
        Msg("! [ShaderCache] Cache version mismatch for %s%s (expected %u, got %u)",
            shaderName, extension, CACHE_VERSION, version);
        FS.r_close(reader);
        m_stats.misses++;
        return false;
    }

    if (storedHash != sourceHash)
    {
        Msg("! [ShaderCache] Source hash mismatch for %s%s (expected %08X, got %08X)",
            shaderName, extension, sourceHash, storedHash);
        FS.r_close(reader);
        m_stats.misses++;
        return false;
    }

    // Read bytecode
    outBytecode.resize(bytecodeSize);
    reader->r(outBytecode.data(), bytecodeSize);
    FS.r_close(reader);

    m_stats.hits++;
    Msg("  ✓ [ShaderCache] Cache HIT: %s%s (%u bytes)", shaderName, extension, bytecodeSize);
    return true;
}

void ShaderCache::Save(
    const char* shaderName,
    const char* extension,
    u32 sourceHash,
    const xr_vector<u8>& bytecode)
{
    string_path cachePath;
    GetCachePath(shaderName, extension, sourceHash, cachePath);

    // Write cache file (directory will be created automatically by w_open)
    IWriter* writer = FS.w_open("$app_data_root$", cachePath);
    if (!writer)
    {
        Msg("! [ShaderCache] Failed to create cache file: %s", cachePath);
        return;
    }

    // Write header
    writer->w_u32(CACHE_VERSION);
    writer->w_u32(sourceHash);
    writer->w_u32(static_cast<u32>(bytecode.size()));

    // Write bytecode
    writer->w(bytecode.data(), bytecode.size());
    FS.w_close(writer);

    m_stats.saves++;
    Msg("  ✓ [ShaderCache] Saved: %s%s (%u bytes)", shaderName, extension, (u32)bytecode.size());
}

u32 ShaderCache::ComputeHash(const char* source, size_t sourceLen)
{
    return crc32(source, sourceLen);
}

u32 ShaderCache::ComputeHash(const char* source, size_t sourceLen, const char* macros)
{
    // Combine source and macros into single hash
    u32 sourceHash = crc32(source, sourceLen);
    u32 macroHash = macros ? crc32(macros, xr_strlen(macros)) : 0;

    // Simple hash combination (XOR + rotate)
    return sourceHash ^ ((macroHash << 16) | (macroHash >> 16));
}

} // namespace xray::render::framegraph
