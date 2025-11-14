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
    xr_vector<u8>& outBytecode,
    ExtractedReflection* outReflection)
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

    // Read reflection if requested and available
    if (outReflection && reader->elapsed() < reader->length())
    {
        u8 hasReflection = reader->r_u8();
        if (hasReflection)
        {
            if (!DeserializeReflection(reader, *outReflection))
            {
                Msg("! [ShaderCache] Failed to deserialize reflection for %s%s", shaderName, extension);
                FS.r_close(reader);
                m_stats.misses++;
                return false;
            }
            Msg("  ✓ [ShaderCache] Cache HIT (with reflection): %s%s (%u bytes)", shaderName, extension, bytecodeSize);
        }
        else
        {
            Msg("  ✓ [ShaderCache] Cache HIT (no reflection): %s%s (%u bytes)", shaderName, extension, bytecodeSize);
        }
    }
    else
    {
        Msg("  ✓ [ShaderCache] Cache HIT: %s%s (%u bytes)", shaderName, extension, bytecodeSize);
    }

    FS.r_close(reader);
    m_stats.hits++;
    return true;
}

void ShaderCache::Save(
    const char* shaderName,
    const char* extension,
    u32 sourceHash,
    const xr_vector<u8>& bytecode,
    const ExtractedReflection* reflection)
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

    // Write reflection if provided
    if (reflection)
    {
        writer->w_u8(1);  // Has reflection flag
        SerializeReflection(writer, *reflection);
        Msg("  ✓ [ShaderCache] Saved (with reflection): %s%s (%u bytes)", shaderName, extension, (u32)bytecode.size());
    }
    else
    {
        writer->w_u8(0);  // No reflection flag
        Msg("  ✓ [ShaderCache] Saved: %s%s (%u bytes)", shaderName, extension, (u32)bytecode.size());
    }

    FS.w_close(writer);
    m_stats.saves++;
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

void ShaderCache::SerializeReflection(IWriter* writer, const ExtractedReflection& reflection)
{
    // ═══════════════════════════════════════════════════
    //  SERIALIZE VERTEX INPUT SIGNATURE
    // ═══════════════════════════════════════════════════
    writer->w_u32(static_cast<u32>(reflection.vertexInputSignature.elements.size()));
    for (const auto& elem : reflection.vertexInputSignature.elements)
    {
        // Write semantic name
        u32 nameLen = elem.semanticName.size();
        writer->w_u32(nameLen);
        if (nameLen > 0)
            writer->w(elem.semanticName.c_str(), nameLen);

        // Write element data
        writer->w_u32(elem.semanticIndex);
        writer->w_u32(static_cast<u32>(elem.format));  // nvrhi::Format enum
        writer->w_u32(elem.inputSlot);
    }

    // ═══════════════════════════════════════════════════
    //  SERIALIZE CONSTANT BUFFERS
    // ═══════════════════════════════════════════════════
    writer->w_u32(static_cast<u32>(reflection.constantBuffers.buffers.size()));
    for (const auto& cb : reflection.constantBuffers.buffers)
    {
        // Write buffer name
        u32 nameLen = cb.name.size();
        writer->w_u32(nameLen);
        if (nameLen > 0)
            writer->w(cb.name.c_str(), nameLen);

        // Write buffer data
        writer->w_u32(cb.slot);
        writer->w_u32(cb.size);
    }

    // ═══════════════════════════════════════════════════
    //  SERIALIZE RT BINDINGS
    // ═══════════════════════════════════════════════════
    writer->w_u32(static_cast<u32>(reflection.rtBindings.phase));  // RenderPhase enum

    // Input textures
    writer->w_u32(static_cast<u32>(reflection.rtBindings.inputTextures.size()));
    for (const auto& tex : reflection.rtBindings.inputTextures)
    {
        u32 nameLen = tex.name.size();
        writer->w_u32(nameLen);
        if (nameLen > 0)
            writer->w(tex.name.c_str(), nameLen);
        writer->w_u32(tex.slot);
    }

    // Samplers
    writer->w_u32(static_cast<u32>(reflection.rtBindings.samplers.size()));
    for (const auto& smp : reflection.rtBindings.samplers)
    {
        u32 nameLen = smp.name.size();
        writer->w_u32(nameLen);
        if (nameLen > 0)
            writer->w(smp.name.c_str(), nameLen);
        writer->w_u32(smp.slot);
    }

    // Output RTs
    writer->w_u32(static_cast<u32>(reflection.rtBindings.outputRTs.size()));
    for (const auto& rt : reflection.rtBindings.outputRTs)
    {
        writer->w_u32(rt.slot);
        writer->w_u32(static_cast<u32>(rt.semantic));  // RTSemantic enum

        u32 descLen = rt.formatDesc.size();
        writer->w_u32(descLen);
        if (descLen > 0)
            writer->w(rt.formatDesc.c_str(), descLen);
    }

    // Depth output
    writer->w_u8(reflection.rtBindings.hasDepthOutput ? 1 : 0);

    // Shader name
    u32 shaderNameLen = reflection.rtBindings.shaderName.size();
    writer->w_u32(shaderNameLen);
    if (shaderNameLen > 0)
        writer->w(reflection.rtBindings.shaderName.c_str(), shaderNameLen);
}

bool ShaderCache::DeserializeReflection(IReader* reader, ExtractedReflection& outReflection)
{
    try
    {
        // ═══════════════════════════════════════════════════
        //  DESERIALIZE VERTEX INPUT SIGNATURE
        // ═══════════════════════════════════════════════════
        u32 elemCount = reader->r_u32();
        outReflection.vertexInputSignature.elements.resize(elemCount);
        for (u32 i = 0; i < elemCount; ++i)
        {
            auto& elem = outReflection.vertexInputSignature.elements[i];

            // Read semantic name
            u32 nameLen = reader->r_u32();
            if (nameLen > 0)
            {
                xr_string name;
                name.resize(nameLen);
                reader->r(&name[0], nameLen);
                elem.semanticName = name.c_str();
            }

            // Read element data
            elem.semanticIndex = reader->r_u32();
            elem.format = static_cast<nvrhi::Format>(reader->r_u32());
            elem.inputSlot = reader->r_u32();
        }

        // ═══════════════════════════════════════════════════
        //  DESERIALIZE CONSTANT BUFFERS
        // ═══════════════════════════════════════════════════
        u32 cbCount = reader->r_u32();
        outReflection.constantBuffers.buffers.resize(cbCount);
        for (u32 i = 0; i < cbCount; ++i)
        {
            auto& cb = outReflection.constantBuffers.buffers[i];

            // Read buffer name
            u32 nameLen = reader->r_u32();
            if (nameLen > 0)
            {
                xr_string name;
                name.resize(nameLen);
                reader->r(&name[0], nameLen);
                cb.name = name.c_str();
            }

            // Read buffer data
            cb.slot = reader->r_u32();
            cb.size = reader->r_u32();
        }

        // ═══════════════════════════════════════════════════
        //  DESERIALIZE RT BINDINGS
        // ═══════════════════════════════════════════════════
        outReflection.rtBindings.phase = static_cast<RenderPhase>(reader->r_u32());

        // Input textures
        u32 texCount = reader->r_u32();
        outReflection.rtBindings.inputTextures.resize(texCount);
        for (u32 i = 0; i < texCount; ++i)
        {
            auto& tex = outReflection.rtBindings.inputTextures[i];

            u32 nameLen = reader->r_u32();
            if (nameLen > 0)
            {
                xr_string name;
                name.resize(nameLen);
                reader->r(&name[0], nameLen);
                tex.name = name.c_str();
            }
            tex.slot = reader->r_u32();
        }

        // Samplers
        u32 smpCount = reader->r_u32();
        outReflection.rtBindings.samplers.resize(smpCount);
        for (u32 i = 0; i < smpCount; ++i)
        {
            auto& smp = outReflection.rtBindings.samplers[i];

            u32 nameLen = reader->r_u32();
            if (nameLen > 0)
            {
                xr_string name;
                name.resize(nameLen);
                reader->r(&name[0], nameLen);
                smp.name = name.c_str();
            }
            smp.slot = reader->r_u32();
        }

        // Output RTs
        u32 rtCount = reader->r_u32();
        outReflection.rtBindings.outputRTs.resize(rtCount);
        for (u32 i = 0; i < rtCount; ++i)
        {
            auto& rt = outReflection.rtBindings.outputRTs[i];

            rt.slot = reader->r_u32();
            rt.semantic = static_cast<ShaderRTBindings::RTSemantic>(reader->r_u32());

            u32 descLen = reader->r_u32();
            if (descLen > 0)
            {
                xr_string desc;
                desc.resize(descLen);
                reader->r(&desc[0], descLen);
                rt.formatDesc = desc.c_str();
            }
        }

        // Depth output
        outReflection.rtBindings.hasDepthOutput = reader->r_u8() != 0;

        // Shader name
        u32 shaderNameLen = reader->r_u32();
        if (shaderNameLen > 0)
        {
            xr_string name;
            name.resize(shaderNameLen);
            reader->r(&name[0], shaderNameLen);
            outReflection.rtBindings.shaderName = name.c_str();
        }

        return true;
    }
    catch (...)
    {
        Msg("! [ShaderCache] Exception during reflection deserialization");
        return false;
    }
}

} // namespace xray::render::framegraph
