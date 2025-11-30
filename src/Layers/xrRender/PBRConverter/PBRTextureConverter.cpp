// xrRender/PBR/PBRTextureConverter.cpp
// Phase 2.5: PBR Texture Conversion Implementation

#include "stdafx.h"
#include "PBRTextureConverter.h"
#include "ONNXModelRunner.h"  // AI-based conversion
#include "xrCore/FS.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include "Layers/xrRender/ResourceManager/DDSLoader.h"
#include "Layers/xrRender/ETextureParams.h"  // For .thm file support
#include <atomic>
#include <algorithm>

using namespace xray::render::RENDER_NAMESPACE;
namespace xray::render::pbr {

// Runtime PBR flag
extern ENGINE_API int ps_r4_use_pbr;

// ══════════════════════════════════════════════════════════
//  HELPER: Software BC1/BC3 Decompression (DXT1/DXT5 → RGBA8)
// ══════════════════════════════════════════════════════════

// Decompress BC3 alpha block → 16 alpha values
static void DecompressBC3AlphaBlock(const u8* blockData, u8* outAlpha) {
    // Read alpha endpoints
    u8 alpha0 = blockData[0];
    u8 alpha1 = blockData[1];

    // Build alpha palette (8 values)
    u8 alphaPalette[8];
    alphaPalette[0] = alpha0;
    alphaPalette[1] = alpha1;

    if (alpha0 > alpha1) {
        // 8-alpha mode: 6 interpolated values
        alphaPalette[2] = (6 * alpha0 + 1 * alpha1) / 7;
        alphaPalette[3] = (5 * alpha0 + 2 * alpha1) / 7;
        alphaPalette[4] = (4 * alpha0 + 3 * alpha1) / 7;
        alphaPalette[5] = (3 * alpha0 + 4 * alpha1) / 7;
        alphaPalette[6] = (2 * alpha0 + 5 * alpha1) / 7;
        alphaPalette[7] = (1 * alpha0 + 6 * alpha1) / 7;
    } else {
        // 6-alpha mode: 4 interpolated values + transparent + opaque
        alphaPalette[2] = (4 * alpha0 + 1 * alpha1) / 5;
        alphaPalette[3] = (3 * alpha0 + 2 * alpha1) / 5;
        alphaPalette[4] = (2 * alpha0 + 3 * alpha1) / 5;
        alphaPalette[5] = (1 * alpha0 + 4 * alpha1) / 5;
        alphaPalette[6] = 0;    // Transparent
        alphaPalette[7] = 255;  // Opaque
    }

    // Read 48 bits of indices (16 pixels × 3 bits each)
    u64 indices = 0;
    indices |= static_cast<u64>(blockData[2]);
    indices |= static_cast<u64>(blockData[3]) << 8;
    indices |= static_cast<u64>(blockData[4]) << 16;
    indices |= static_cast<u64>(blockData[5]) << 24;
    indices |= static_cast<u64>(blockData[6]) << 32;
    indices |= static_cast<u64>(blockData[7]) << 40;

    // Write 16 alpha values
    for (u32 i = 0; i < 16; ++i) {
        u32 index = static_cast<u32>((indices >> (i * 3)) & 0x7);
        outAlpha[i] = alphaPalette[index];
    }
}

// Decompress a single BC1 (DXT1) block → 16 RGBA pixels
static void DecompressBC1Block(const u8* blockData, u8* outPixels, bool hasAlpha) {
    // Read color endpoints (565 format)
    u16 color0 = blockData[0] | (blockData[1] << 8);
    u16 color1 = blockData[2] | (blockData[3] << 8);

    // Extract RGB (565 → 888)
    u8 r0 = ((color0 >> 11) & 0x1F) * 255 / 31;
    u8 g0 = ((color0 >> 5) & 0x3F) * 255 / 63;
    u8 b0 = (color0 & 0x1F) * 255 / 31;

    u8 r1 = ((color1 >> 11) & 0x1F) * 255 / 31;
    u8 g1 = ((color1 >> 5) & 0x3F) * 255 / 63;
    u8 b1 = (color1 & 0x1F) * 255 / 31;

    // Build color palette
    u8 palette[4][4];
    palette[0][0] = r0; palette[0][1] = g0; palette[0][2] = b0; palette[0][3] = 255;
    palette[1][0] = r1; palette[1][1] = g1; palette[1][2] = b1; palette[1][3] = 255;

    if (color0 > color1 || !hasAlpha) {
        // 4-color mode
        palette[2][0] = (2 * r0 + r1) / 3; palette[2][1] = (2 * g0 + g1) / 3; palette[2][2] = (2 * b0 + b1) / 3; palette[2][3] = 255;
        palette[3][0] = (r0 + 2 * r1) / 3; palette[3][1] = (g0 + 2 * g1) / 3; palette[3][2] = (b0 + 2 * b1) / 3; palette[3][3] = 255;
    } else {
        // 3-color mode + transparent
        palette[2][0] = (r0 + r1) / 2; palette[2][1] = (g0 + g1) / 2; palette[2][2] = (b0 + b1) / 2; palette[2][3] = 255;
        palette[3][0] = 0; palette[3][1] = 0; palette[3][2] = 0; palette[3][3] = 0;  // Transparent
    }

    // Read indices (2 bits per pixel, 32 bits total)
    u32 indices = blockData[4] | (blockData[5] << 8) | (blockData[6] << 16) | (blockData[7] << 24);

    // Write 4x4 pixels
    for (u32 i = 0; i < 16; ++i) {
        u32 index = (indices >> (i * 2)) & 0x3;
        outPixels[i * 4 + 0] = palette[index][0];
        outPixels[i * 4 + 1] = palette[index][1];
        outPixels[i * 4 + 2] = palette[index][2];
        outPixels[i * 4 + 3] = palette[index][3];
    }
}

// Decompress DXT1/DXT5 texture → RGBA8
static xr_vector<u8> DecompressDDS(const resources::DDSData& ddsData, u32& outWidth, u32& outHeight) {
    xr_vector<u8> result;

    if (!ddsData.isValid || ddsData.mipLevels.empty()) {
        return result;
    }

    const auto& mip0 = ddsData.mipLevels[0];
    outWidth = mip0.width;
    outHeight = mip0.height;

    const u32 pixelCount = outWidth * outHeight;
    result.resize(pixelCount * 4);  // RGBA8

    // Check format (cast enum to int for comparison)
    bool isDXT1 = (static_cast<int>(ddsData.desc.format) == 56);  // BC1/DXT1
    bool isDXT5 = (static_cast<int>(ddsData.desc.format) == 60);  // BC3/DXT5

    if (!isDXT1 && !isDXT5) {
        Msg("! [PBRTextureConverter] Unsupported format: %d (only DXT1/DXT5 supported)", static_cast<int>(ddsData.desc.format));
        return result;
    }

    const u32 blockSize = isDXT1 ? 8 : 16;  // DXT1=8 bytes, DXT5=16 bytes
    const u32 blocksX = (outWidth + 3) / 4;
    const u32 blocksY = (outHeight + 3) / 4;

    const u8* srcData = mip0.data;

    for (u32 by = 0; by < blocksY; ++by) {
        for (u32 bx = 0; bx < blocksX; ++bx) {
            const u8* blockData = srcData + (by * blocksX + bx) * blockSize;
            u8 blockPixels[64];  // 4x4 RGBA

            if (isDXT5) {
                // DXT5: 8 bytes alpha + 8 bytes color
                u8 alphaValues[16];
                DecompressBC3AlphaBlock(blockData, alphaValues);
                DecompressBC1Block(blockData + 8, blockPixels, false);

                // Apply alpha values to RGB pixels
                for (u32 i = 0; i < 16; ++i) {
                    blockPixels[i * 4 + 3] = alphaValues[i];
                }
            } else {
                // DXT1: 8 bytes color only
                DecompressBC1Block(blockData, blockPixels, true);
            }

            // Copy 4x4 block to output
            for (u32 py = 0; py < 4 && (by * 4 + py) < outHeight; ++py) {
                for (u32 px = 0; px < 4 && (bx * 4 + px) < outWidth; ++px) {
                    u32 srcIdx = (py * 4 + px) * 4;
                    u32 dstIdx = ((by * 4 + py) * outWidth + (bx * 4 + px)) * 4;
                    result[dstIdx + 0] = blockPixels[srcIdx + 0];
                    result[dstIdx + 1] = blockPixels[srcIdx + 1];
                    result[dstIdx + 2] = blockPixels[srcIdx + 2];
                    result[dstIdx + 3] = blockPixels[srcIdx + 3];
                }
            }
        }
    }

    return result;
}

#ifdef USE_AI_PBR
// Global AI pipeline (initialized once, reused for all conversions)
static xr_unique_ptr<PBRPipeline> g_ai_pipeline;
static bool g_ai_available = false;

// Mutex to serialize AI inference (ONNX Runtime sessions are NOT thread-safe)
static Lock g_ai_pipeline_mutex;
#endif

// ══════════════════════════════════════════════════════════
//  HELPER: Extract base name from texture path
// ══════════════════════════════════════════════════════════
// "wood\oak_d.dds" → "wood\oak"
// "metal_rusty_s.dds" → "metal_rusty"

static xr_string ExtractBaseName(const xr_string& path) {
    xr_string result = path;

    // Remove extension
    const auto dot = result.find_last_of('.');
    if (dot != xr_string::npos) {
        result = result.substr(0, dot);
    }

    // Remove suffix (_d, _s, _g, _bump, _bump#)
    const auto suffixes = { "_d", "_s", "_g", "_bump", "_bump#" };
    for (const auto& suffix : suffixes) {
        const size_t suffix_len = xr_strlen(suffix);
        if (result.size() >= suffix_len) {
            const xr_string ending = result.substr(result.size() - suffix_len);
            if (ending == suffix) {
                result = result.substr(0, result.size() - suffix_len);
                break;
            }
        }
    }

    return result;
}

// ══════════════════════════════════════════════════════════
//  HELPER: Check if file exists
// ══════════════════════════════════════════════════════════

static bool FileExists(const char* root_alias, const char* relative_path) {
    string_path full_path;
    FS.update_path(full_path, root_alias, relative_path);
    return FS.exist(full_path);
}

// ══════════════════════════════════════════════════════════
//  HELPER: Get file metadata
// ══════════════════════════════════════════════════════════

static FileLocation MakeFileLocation(
    const xr_string& root_alias,
    const FS_File& file)
{
    FileLocation loc;
    loc.root_alias = root_alias;
    loc.relative_path = file.name.c_str();
    loc.file_size = file.size >= 0 ? static_cast<std::int64_t>(file.size) : 0;
    loc.modified_time_seconds = file.time_write >= 0 ? static_cast<std::int64_t>(file.time_write) : 0;
    return loc;
}

// ══════════════════════════════════════════════════════════
//  BUILD TEXTURE INVENTORY (Phase 2.5.2)
// ══════════════════════════════════════════════════════════
// Scans filesystem for diffuse textures (_d.dds) and finds
// matching specular/gloss/normal textures

// ══════════════════════════════════════════════════════════
//  FOLDER BLACKLIST - Skip folders with special texture semantics
// ══════════════════════════════════════════════════════════
// These folders contain textures where RGBA channels have special meanings
// (e.g., terrain masks use RGBA for 4-layer blending) and should NOT be
// processed by the PBR converter which may corrupt alpha channels.

static const char* const FOLDER_BLACKLIST[] = {
    "editor",
    "fx",
    "glow"
    "grad",
    "internal",
    "lights",
    "pfx",
    "sky",          // Sky textures
    "ui",           // UI textures
};

static bool IsInBlacklistedFolder(const xr_string& path) {
    for (const char* folder : FOLDER_BLACKLIST) {
        // Check if path starts with "folder\" or "folder/"
        xr_string prefix1 = xr_string(folder) + "\\";
        xr_string prefix2 = xr_string(folder) + "/";
        if (path.find(prefix1) == 0 || path.find(prefix2) == 0) {
            return true;
        }
        // Also check if folder appears anywhere in path (subfolder)
        xr_string infix1 = xr_string("\\") + folder + "\\";
        xr_string infix2 = xr_string("/") + folder + "/";
        if (path.find(infix1) != xr_string::npos || path.find(infix2) != xr_string::npos) {
            return true;
        }
    }
    return false;
}

TextureInventory BuildTextureInventory(const TextureScanConfig& config) {
    TextureInventory inventory;

    // Map: base_name → asset
    xr_map<xr_string, LegacyTextureAsset> asset_map;

    for (const auto& root : config.texture_roots) {
        // ═══════════════════════════════════════════════════════
        //  SCAN FOR ALL DDS TEXTURES
        // ═══════════════════════════════════════════════════════
        FS_FileSet diffuse_files;

        // Recursive or non-recursive based on config
        // NOTE: FS_RootOnly means "don't recurse" - so we add it when recursive=false!
        const u32 flags = FS_ListFiles | (config.recursive ? 0 : FS_RootOnly);
        FS.file_list(diffuse_files, root.c_str(), flags, "*.dds");

        for (const auto& file : diffuse_files) {
            const xr_string file_name = file.name.c_str();

            // Skip special shader test textures (start with $)
            if (!file_name.empty() && file_name[0] == '$') {
                continue;  // Skip shader test textures like $alphadxt1, $shadertest, etc.
            }

            // Skip blacklisted folders (terrain, levels, etc.)
            if (IsInBlacklistedFolder(file_name)) {
                continue;  // Skip textures in folders with special RGBA semantics
            }

            // Skip files that are clearly NOT base textures
            // We want to find base diffuse/albedo textures, which can be:
            // - Vanilla: wood.dds (no suffix)
            // - Modded: wood_d.dds (diffuse suffix)
            // Skip: bump maps, normal maps, metallic, roughness, etc.
            if (file_name.find("_bump") != xr_string::npos ||
                file_name.find("_bump#") != xr_string::npos ||
                file_name.find("_normal") != xr_string::npos ||
                file_name.find("_metallic") != xr_string::npos ||
                file_name.find("_roughness") != xr_string::npos ||
                file_name.find("_ao") != xr_string::npos ||
                file_name.find("_parallax") != xr_string::npos ||
                file_name.find("_s.dds") != xr_string::npos ||  // Skip specular maps
                file_name.find("_g.dds") != xr_string::npos) {  // Skip gloss maps
                continue;  // Skip these utility textures
            }

            // This is a potential base texture (either vanilla or modded)
            // Examples: wood.dds, wood_d.dds, metal_rusty.dds, etc.

            // Extract base name
            const xr_string base_name = ExtractBaseName(file_name);
            if (base_name.empty()) {
                continue;
            }

            // Create or get asset
            auto& asset = asset_map[base_name];
            asset.base_name = base_name;
            asset.diffuse = MakeFileLocation(root, file);
            asset.total_size_bytes = asset.diffuse.file_size;
            asset.latest_modified_time = asset.diffuse.modified_time_seconds;

            // ═══════════════════════════════════════════════════════
            //  LOOK FOR MATCHING TEXTURES
            // ═══════════════════════════════════════════════════════

            // Specular (_s.dds) - for heuristic fallback
            xr_string spec_path = base_name + "_s.dds";
            if (FileExists(root.c_str(), spec_path.c_str())) {
                asset.has_specular = true;
                asset.specular.root_alias = root;
                asset.specular.relative_path = spec_path;
            }

            // Gloss (_g.dds) - for heuristic fallback
            xr_string gloss_path = base_name + "_g.dds";
            if (FileExists(root.c_str(), gloss_path.c_str())) {
                asset.has_gloss = true;
                asset.gloss.root_alias = root;
                asset.gloss.relative_path = gloss_path;
            }

            // Normal (_bump.dds or _bump#.dds) - for AI conversion
            xr_string normal_path = base_name + "_bump.dds";
            if (FileExists(root.c_str(), normal_path.c_str())) {
                asset.has_normal = true;
                asset.normal.root_alias = root;
                asset.normal.relative_path = normal_path;
            } else {
                // Try alternate naming
                normal_path = base_name + "_bump#.dds";
                if (FileExists(root.c_str(), normal_path.c_str())) {
                    asset.has_normal = true;
                    asset.normal.root_alias = root;
                    asset.normal.relative_path = normal_path;
                }
            }
        }
    }

    // Convert map to vector
    inventory.assets.reserve(asset_map.size());
    for (auto& [base_name, asset] : asset_map) {
        inventory.assets.emplace_back(std::move(asset));

        // Update statistics
        inventory.total_textures++;
        if (asset.has_specular) inventory.textures_with_specular++;
        if (asset.has_gloss) inventory.textures_with_gloss++;
        inventory.total_size_bytes += asset.total_size_bytes;
    }

    Msg("[PBRTextureConverter] Scanned %u textures (%u with specular, %u with gloss)",
        inventory.total_textures,
        inventory.textures_with_specular,
        inventory.textures_with_gloss);

    return inventory;
}

// ══════════════════════════════════════════════════════════
//  COMPUTE INVENTORY DIGEST (Change Detection)
// ══════════════════════════════════════════════════════════
// Similar to OGF digest: Hash all file sizes and timestamps

xr_string ComputeTextureInventoryDigest(const TextureInventory& inventory) {
    // Build string with all metadata
    xr_string digest_input;
    digest_input.reserve(inventory.assets.size() * 128);

    for (const auto& asset : inventory.assets) {
        digest_input.append(asset.base_name);
        digest_input.append(":");
        digest_input.append(std::to_string(asset.total_size_bytes));
        digest_input.append(":");
        digest_input.append(std::to_string(asset.latest_modified_time));
        digest_input.append("|");
    }

    // Compute SHA256 hash
    // TODO: Use X-Ray's crypto API (xrCore/crypto/crypto.h)
    // For now, use simple hash
    const u32 simple_hash = crc32(digest_input.c_str(), digest_input.size());

    char hex_buffer[16];
    xr_sprintf(hex_buffer, sizeof(hex_buffer), "%08x", simple_hash);
    return xr_string(hex_buffer);
}

// ══════════════════════════════════════════════════════════
//  HELPER: Check if .thm needs PBR update
// ══════════════════════════════════════════════════════════
// Returns true if .thm doesn't exist or lacks PBR texture references

static bool THMNeedsPBRUpdate(
    const char* root_alias,
    const xr_string& base_name,
    const xr_string& metallic_name,
    const xr_string& roughness_name,
    const xr_string& ao_name,
    const xr_string& parallax_name)
{
    xr_string thm_path = base_name + ".thm";

    string_path full_path;
    FS.update_path(full_path, root_alias, thm_path.c_str());

    if (!FS.exist(full_path)) {
        Msg("* [PBRTextureConverter] THMNeedsPBRUpdate: %s doesn't exist, needs creation", full_path);
        return true;  // .thm doesn't exist
    }

    // Load existing .thm and check PBR fields
    IReader* reader = FS.r_open(root_alias, thm_path.c_str());
    if (!reader) {
        Msg("* [PBRTextureConverter] THMNeedsPBRUpdate: Can't read %s", full_path);
        return true;  // Can't read, needs update
    }

    STextureParams params;
    params.Clear();
    params.Load(*reader);
    FS.r_close(reader);

    // Check if PBR names match expected values
    if (params.metallic_name != metallic_name.c_str() ||
        params.roughness_name != roughness_name.c_str() ||
        params.ao_name != ao_name.c_str() ||
        params.parallax_name != parallax_name.c_str()) {
        Msg("* [PBRTextureConverter] THMNeedsPBRUpdate: %s has outdated PBR fields", full_path);
        return true;  // PBR fields don't match
    }

    return false;  // .thm is up to date
}

// ══════════════════════════════════════════════════════════
//  VERIFY PBR OUTPUTS (Check if conversion needed)
// ══════════════════════════════════════════════════════════

bool VerifyPBROutputs(
    const TextureInventory& inventory,
    const PBRConversionParams& params)
{
    u32 missing_textures = 0;
    u32 missing_thm = 0;

    for (const auto& asset : inventory.assets) {
        // Check if metallic/roughness outputs exist
        xr_string metallic_path = asset.base_name + "_metallic.dds";
        xr_string roughness_path = asset.base_name + "_roughness.dds";

        if (!FileExists(params.output_root.c_str(), metallic_path.c_str()) ||
            !FileExists(params.output_root.c_str(), roughness_path.c_str())) {
            missing_textures++;
        } else {
            // Textures exist - check if .thm exists with PBR fields
            xr_string metallic_name = asset.base_name + "_metallic";
            xr_string roughness_name = asset.base_name + "_roughness";
            xr_string ao_name = asset.base_name + "_ao";
            xr_string parallax_name = asset.base_name + "_parallax";

            if (THMNeedsPBRUpdate(params.output_root.c_str(), asset.base_name,
                                  metallic_name, roughness_name, ao_name, parallax_name)) {
                missing_thm++;
            }
        }
    }

    if (missing_textures > 0) {
        Msg("[PBRTextureConverter] %u/%u textures need conversion",
            missing_textures, inventory.total_textures);
        return false;  // Need full conversion
    }

    if (missing_thm > 0) {
        Msg("[PBRTextureConverter] %u/%u .thm files need PBR fields updated",
            missing_thm, inventory.total_textures);
        return false;  // Need .thm update pass
    }

    return true;  // All outputs verified
}

// ══════════════════════════════════════════════════════════
//  HELPER: Convert Spec/Gloss → Metallic/Roughness (Phase 2.5.2)
// ══════════════════════════════════════════════════════════
// PBR Conversion Algorithm:
// - Metallic: Extracted from specular reflectance (high specular = metallic)
// - Roughness: Inverted gloss (roughness = 1.0 - gloss)
// - Fallbacks: metallic=0.0 (non-metallic), roughness=0.5 (mid-range)

struct ConvertedPBRTextures {
    xr_vector<u8> albedoData;     // RGBA (4 channels) - preserves original alpha!
    xr_vector<u8> metallicData;   // Single-channel (R8)
    xr_vector<u8> roughnessData;  // Single-channel (R8)
    xr_vector<u8> aoData;         // Single-channel (R8)
    xr_vector<u8> parallaxData;   // Single-channel (R8) - height map
    u32 width = 0;
    u32 height = 0;
    bool success = false;
};

static ConvertedPBRTextures ConvertSpecularGlossToPBR(
    const resources::DDSData* diffuseData,
    const resources::DDSData* specularData,
    const resources::DDSData* glossData,
    const PBRConversionParams& params)
{
    ConvertedPBRTextures result;

    // Validate diffuse (required for dimensions)
    if (!diffuseData || !diffuseData->isValid || diffuseData->mipLevels.empty()) {
        return result;
    }

    const auto& baseMip = diffuseData->mipLevels[0];
    result.width = baseMip.width;
    result.height = baseMip.height;

    const u32 pixelCount = result.width * result.height;
    result.metallicData.resize(pixelCount, static_cast<u8>(params.default_metallic * 255.0f));
    result.roughnessData.resize(pixelCount, static_cast<u8>(params.default_roughness * 255.0f));

    // ═══════════════════════════════════════════════════════
    //  EXTRACT METALLIC FROM SPECULAR
    // ═══════════════════════════════════════════════════════
    if (specularData && specularData->isValid && !specularData->mipLevels.empty()) {
        const auto& specMip = specularData->mipLevels[0];

        // Ensure dimensions match
        if (specMip.width == result.width && specMip.height == result.height) {
            const u8* specPixels = specMip.data;

            // Assume RGBA format (4 bytes per pixel) - use red channel
            // Metallic workflow: High specular reflectance = metallic surface
            for (u32 i = 0; i < pixelCount; ++i) {
                const u8 specR = specPixels[i * 4 + 0];  // Red channel
                result.metallicData[i] = specR;
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  EXTRACT ROUGHNESS FROM GLOSS (INVERTED)
    // ═══════════════════════════════════════════════════════
    if (glossData && glossData->isValid && !glossData->mipLevels.empty()) {
        const auto& glossMip = glossData->mipLevels[0];

        // Ensure dimensions match
        if (glossMip.width == result.width && glossMip.height == result.height) {
            const u8* glossPixels = glossMip.data;

            // Roughness = 1.0 - Gloss (invert)
            for (u32 i = 0; i < pixelCount; ++i) {
                const u8 glossValue = glossPixels[i * 4 + 0];  // Red channel
                result.roughnessData[i] = 255 - glossValue;    // Invert
            }
        }
    }

    result.success = true;
    return result;
}

#ifdef USE_AI_PBR

// ══════════════════════════════════════════════════════════
//  HELPER: Initialize AI Pipeline (Once)
// ══════════════════════════════════════════════════════════

static bool InitializeAIPipeline() {
    if (g_ai_pipeline) {
        return true;  // Already initialized
    }

    // Initialize pipeline
    PBRPipelineConfig config;
    config.use_gpu = true;
    config.verbose = true;  // Set to true for debugging

    g_ai_pipeline.reset(xr_new<PBRPipeline>());
    if (!g_ai_pipeline->Initialize(config)) {
        Msg("! [PBRTextureConverter] Failed to initialize AI pipeline");
        g_ai_pipeline.reset();
        g_ai_available = false;
        return false;
    }

    g_ai_available = true;
    Msg("[PBRTextureConverter] AI pipeline initialized successfully");
    return true;
}

// ══════════════════════════════════════════════════════════
//  HELPER: Convert Using AI Pipeline
// ══════════════════════════════════════════════════════════

static ConvertedPBRTextures ConvertWithAI(
    const resources::DDSData* diffuseData,
    const resources::DDSData* normalData,
    const PBRConversionParams& params)
{
    ConvertedPBRTextures result;

    // Validate inputs
    if (!diffuseData || !diffuseData->isValid || diffuseData->mipLevels.empty()) {
        return result;
    }
    if (!normalData || !normalData->isValid || normalData->mipLevels.empty()) {
        return result;
    }

    // Decompress DXT textures → RGBA8
    u32 diffuseWidth, diffuseHeight, normalWidth, normalHeight;
    xr_vector<u8> diffuseRGB = DecompressDDS(*diffuseData, diffuseWidth, diffuseHeight);
    xr_vector<u8> normalRGB = DecompressDDS(*normalData, normalWidth, normalHeight);

    if (diffuseRGB.empty() || normalRGB.empty()) {
        Msg("! [PBRTextureConverter] Failed to decompress textures");
        return result;
    }

    Msg("~ [PBRTextureConverter] Decompressed dimensions: diffuse %ux%u, normal %ux%u",
        diffuseWidth, diffuseHeight, normalWidth, normalHeight);

    result.width = diffuseWidth;
    result.height = diffuseHeight;

    // Handle dimension mismatch (upscale normal map if needed)
    if (normalWidth != diffuseWidth || normalHeight != diffuseHeight) {
        Msg("~ [PBRTextureConverter] Upscaling normal map: %ux%u → %ux%u",
            normalWidth, normalHeight, diffuseWidth, diffuseHeight);

        // Simple nearest-neighbor upscale (TODO: use bilinear)
        xr_vector<u8> upscaledNormal(diffuseWidth * diffuseHeight * 4);
        for (u32 y = 0; y < diffuseHeight; ++y) {
            for (u32 x = 0; x < diffuseWidth; ++x) {
                u32 srcX = x * normalWidth / diffuseWidth;
                u32 srcY = y * normalHeight / diffuseHeight;
                u32 srcIdx = (srcY * normalWidth + srcX) * 4;
                u32 dstIdx = (y * diffuseWidth + x) * 4;
                upscaledNormal[dstIdx + 0] = normalRGB[srcIdx + 0];
                upscaledNormal[dstIdx + 1] = normalRGB[srcIdx + 1];
                upscaledNormal[dstIdx + 2] = normalRGB[srcIdx + 2];
                upscaledNormal[dstIdx + 3] = normalRGB[srcIdx + 3];
            }
        }
        normalRGB = std::move(upscaledNormal);
    }

    // Run AI pipeline with uncompressed RGB data
    // NOTE: ONNX Runtime sessions are NOT thread-safe, so we must serialize access
    Msg("~ [PBRTextureConverter] Calling Process with dimensions: width=%u, height=%u",
        result.width, result.height);

    PBRPipelineOutputs outputs;
    {
        ScopeLock lock{ &g_ai_pipeline_mutex };
        outputs = g_ai_pipeline->Process(
            diffuseRGB.data(),
            normalRGB.data(),
            result.width,
            result.height
        );
    }

    if (!outputs.success) {
        Msg("! [PBRTextureConverter] AI pipeline failed");
        return result;
    }

    // Convert tensor outputs to u8 data
    // Matching Python processing:
    // - Albedo: NO sigmoid (model has built-in nn.Sigmoid())
    // - All others: YES sigmoid (raw logits from model)
    xr_vector<u8> aiAlbedoRGB = outputs.albedo.ToImageData(false);  // RGB basecolor (NO sigmoid)
    result.metallicData = outputs.metallic.ToImageData(true);   // R8 (sigmoid)
    result.roughnessData = outputs.roughness.ToImageData(true); // R8 (sigmoid)
    result.aoData = outputs.ao.ToImageData(true);               // R8 (sigmoid)
    result.parallaxData = outputs.parallax.ToImageData(true);   // R8 height map (sigmoid)

    // ═══════════════════════════════════════════════════════
    //  PRESERVE ORIGINAL ALPHA CHANNEL
    // ═══════════════════════════════════════════════════════
    // Combine AI-generated RGB albedo with original diffuse alpha.
    // This is critical for textures where alpha has special meaning:
    // - Terrain masks: RGBA channels control 4-layer blending
    // - Transparent textures: alpha controls opacity
    // - Specular in alpha: some workflows store spec/gloss in alpha
    const u32 pixelCount = result.width * result.height;
    result.albedoData.resize(pixelCount * 4);  // RGBA

    for (u32 i = 0; i < pixelCount; ++i) {
        // RGB from AI model
        result.albedoData[i * 4 + 0] = aiAlbedoRGB[i * 3 + 0];  // R
        result.albedoData[i * 4 + 1] = aiAlbedoRGB[i * 3 + 1];  // G
        result.albedoData[i * 4 + 2] = aiAlbedoRGB[i * 3 + 2];  // B
        // Alpha from ORIGINAL diffuse texture (diffuseRGB is actually RGBA from DecompressDDS)
        result.albedoData[i * 4 + 3] = diffuseRGB[i * 4 + 3];   // A - preserved!
    }

    result.success = true;
    return result;
}

#endif // USE_AI_PBR

// ══════════════════════════════════════════════════════════
//  HELPER: Write Single-Channel DDS (Phase 2.5.2)
// ══════════════════════════════════════════════════════════
// Writes R8 format DDS file (single channel, 8-bit)

static xr_vector<u8> GenerateMipLevel(const u8* srcData, u32 srcWidth, u32 srcHeight, u32 channels) {
    // Box filter downsample by 2x2
    const u32 dstWidth = std::max(srcWidth / 2, 1u);
    const u32 dstHeight = std::max(srcHeight / 2, 1u);
    xr_vector<u8> dstData(dstWidth * dstHeight * channels);

    for (u32 y = 0; y < dstHeight; ++y) {
        for (u32 x = 0; x < dstWidth; ++x) {
            // Sample 2x2 pixels from source
            for (u32 c = 0; c < channels; ++c) {
                u32 sum = 0;
                u32 count = 0;

                for (u32 sy = 0; sy < 2 && (y * 2 + sy) < srcHeight; ++sy) {
                    for (u32 sx = 0; sx < 2 && (x * 2 + sx) < srcWidth; ++sx) {
                        sum += srcData[((y * 2 + sy) * srcWidth + (x * 2 + sx)) * channels + c];
                        count++;
                    }
                }

                dstData[(y * dstWidth + x) * channels + c] = static_cast<u8>(sum / count);
            }
        }
    }

    return dstData;
}

static bool WriteRGBADDS(
    const char* root_alias,
    const xr_string& relative_path,
    const u8* rgbaData,
    u32 width,
    u32 height,
    bool generateMipmaps)
{
    // Build DDS header for RGBA8 (32-bit with alpha, like texconv outputs)
    using namespace resources;

    // Calculate mipmap count
    u32 mipCount = 1;
    if (generateMipmaps) {
        u32 w = width, h = height;
        while (w > 1 || h > 1) {
            w = std::max(w / 2, 1u);
            h = std::max(h / 2, 1u);
            mipCount++;
        }
    }

    DDS_HEADER header = {};
    header.dwSize = 124;
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH;
    if (generateMipmaps) {
        header.dwFlags |= DDSD_MIPMAPCOUNT;
    }
    header.dwHeight = height;
    header.dwWidth = width;
    header.dwPitchOrLinearSize = width * 4;  // 4 bytes per pixel (RGBA)
    header.dwDepth = 0;
    header.dwMipMapCount = mipCount;

    // Pixel format: RGBA8 (32-bit with alpha)
    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;  // Red (low byte)
    header.ddspf.dwGBitMask = 0x0000FF00;  // Green
    header.ddspf.dwBBitMask = 0x00FF0000;  // Blue
    header.ddspf.dwABitMask = 0xFF000000;  // Alpha (high byte)

    header.dwCaps = DDSCAPS_TEXTURE;
    if (generateMipmaps) {
        header.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }

    // Write file
    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));
    writer->w(&header, sizeof(header));

    // Write mip 0 - data is already RGBA with preserved alpha!
    const u32 pixelCount = width * height;
    writer->w(rgbaData, pixelCount * 4);

    // Generate and write mipmaps
    if (generateMipmaps) {
        xr_vector<u8> currentMip(rgbaData, rgbaData + pixelCount * 4);
        u32 mipWidth = width;
        u32 mipHeight = height;

        for (u32 mipLevel = 1; mipLevel < mipCount; ++mipLevel) {
            currentMip = GenerateMipLevel(currentMip.data(), mipWidth, mipHeight, 4);
            mipWidth = std::max(mipWidth / 2, 1u);
            mipHeight = std::max(mipHeight / 2, 1u);

            writer->w(currentMip.data(), currentMip.size());
        }
    }

    FS.w_close(writer);
    return true;
}

static bool WriteSingleChannelDDS(
    const char* root_alias,
    const xr_string& relative_path,
    const u8* pixelData,
    u32 width,
    u32 height,
    bool generateMipmaps)
{
    // Build DDS header
    using namespace resources;

    // Calculate mipmap count
    u32 mipCount = 1;
    if (generateMipmaps) {
        u32 w = width, h = height;
        while (w > 1 || h > 1) {
            w = std::max(w / 2, 1u);
            h = std::max(h / 2, 1u);
            mipCount++;
        }
    }

    DDS_HEADER header = {};
    header.dwSize = 124;
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    if (generateMipmaps) {
        header.dwFlags |= DDSD_MIPMAPCOUNT;
    }
    header.dwHeight = height;
    header.dwWidth = width;
    header.dwPitchOrLinearSize = width;  // 1 byte per pixel
    header.dwDepth = 0;
    header.dwMipMapCount = mipCount;

    // Pixel format: R8 (single channel, 8-bit)
    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_LUMINANCE;
    header.ddspf.dwRGBBitCount = 8;
    header.ddspf.dwRBitMask = 0xFF;
    header.ddspf.dwGBitMask = 0;
    header.ddspf.dwBBitMask = 0;
    header.ddspf.dwABitMask = 0;

    header.dwCaps = DDSCAPS_TEXTURE;
    if (generateMipmaps) {
        header.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }

    // Write file using two-parameter VFS pattern
    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    // Write magic number
    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));

    // Write header
    writer->w(&header, sizeof(header));

    // Write mip 0
    const u32 dataSize = width * height;
    writer->w(pixelData, dataSize);

    // Generate and write mipmaps
    if (generateMipmaps) {
        xr_vector<u8> currentMip(pixelData, pixelData + dataSize);
        u32 mipWidth = width;
        u32 mipHeight = height;

        for (u32 mipLevel = 1; mipLevel < mipCount; ++mipLevel) {
            currentMip = GenerateMipLevel(currentMip.data(), mipWidth, mipHeight, 1);
            mipWidth = std::max(mipWidth / 2, 1u);
            mipHeight = std::max(mipHeight / 2, 1u);

            writer->w(currentMip.data(), currentMip.size());
        }
    }

    FS.w_close(writer);
    return true;
}

// ══════════════════════════════════════════════════════════
//  SANITY TEST: Write decompressed DDS to verify decompressor
// ══════════════════════════════════════════════════════════
static bool WriteUncompressedDDS(
    const char* root_alias,
    const xr_string& relative_path,
    const u8* rgbaData,
    u32 width,
    u32 height)
{
    using namespace resources;

    DDS_HEADER header = {};
    header.dwSize = 124;
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    header.dwWidth = width;
    header.dwHeight = height;
    header.dwPitchOrLinearSize = width * 4; // RGBA8
    header.dwDepth = 0;
    header.dwMipMapCount = 1;

    // RGBA8 pixel format (uncompressed)
    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;
    header.ddspf.dwGBitMask = 0x0000FF00;
    header.ddspf.dwBBitMask = 0x00FF0000;
    header.ddspf.dwABitMask = 0xFF000000;

    header.dwCaps = DDSCAPS_TEXTURE;

    // Write file
    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));
    writer->w(&header, sizeof(header));
    writer->w(rgbaData, width * height * 4); // RGBA8

    FS.w_close(writer);
    Msg("* [PBRTextureConverter] Wrote uncompressed DDS: %s/%s (%ux%u)", root_alias, relative_path.c_str(), width, height);
    return true;
}

static void SanityTestDecompressor(const LegacyTextureAsset& asset)
{
    Msg("~ [PBRTextureConverter] SANITY TEST: Decompressing %s...", asset.diffuse.relative_path.c_str());

    // Load DDS
    auto StripDDSExtension = [](const xr_string& path) -> xr_string {
        if (path.size() >= 4 && path.substr(path.size() - 4) == ".dds") {
            return path.substr(0, path.size() - 4);
        }
        return path;
    };

    resources::DDSData diffuseData;
    xr_string diffuse_vfs_path = StripDDSExtension(asset.diffuse.relative_path);
    if (!resources::DDSLoader::LoadFromFile(diffuse_vfs_path.c_str(), diffuseData)) {
        Msg("! [PBRTextureConverter] SANITY TEST: Failed to load diffuse");
        return;
    }

    // Decompress
    u32 width, height;
    xr_vector<u8> rgba = DecompressDDS(diffuseData, width, height);

    if (rgba.empty()) {
        Msg("! [PBRTextureConverter] SANITY TEST: Decompression failed");
        return;
    }

    Msg("* [PBRTextureConverter] SANITY TEST: Decompressed to %ux%u RGBA8 (%u bytes)", width, height, rgba.size());

    // Write to res/gamedata/test_decompress.dds
    WriteUncompressedDDS("$game_data$", "test_decompress_diffuse.dds", rgba.data(), width, height);

    // Also test normal map if available
    if (asset.has_normal) {
        resources::DDSData normalData;
        xr_string normal_vfs_path = StripDDSExtension(asset.normal.relative_path);
        if (resources::DDSLoader::LoadFromFile(normal_vfs_path.c_str(), normalData)) {
            xr_vector<u8> normalRGBA = DecompressDDS(normalData, width, height);
            if (!normalRGBA.empty()) {
                WriteUncompressedDDS("$game_data$", "test_decompress_normal.dds", normalRGBA.data(), width, height);
            }
        }
    }

    Msg("* [PBRTextureConverter] SANITY TEST: Complete - check gamedata/test_decompress_*.dds");
}

// ══════════════════════════════════════════════════════════
//  HELPER: Write/Update .thm file with PBR texture references
// ══════════════════════════════════════════════════════════
// Creates or updates a .thm file to include PBR texture names
// This allows TextureDescrManager to find PBR textures via normal lookup

static bool WriteTHMFile(
    const char* root_alias,
    const xr_string& base_name,
    const xr_string& metallic_name,
    const xr_string& roughness_name,
    const xr_string& ao_name,
    const xr_string& parallax_name)
{
    xr_string thm_path = base_name + ".thm";

    // Try to load existing .thm file
    STextureParams params;
    params.Clear();

    string_path full_path;
    FS.update_path(full_path, root_alias, thm_path.c_str());

    Msg("* [PBRTextureConverter] WriteTHMFile: Writing to %s (resolved: %s)", thm_path.c_str(), full_path);

    if (FS.exist(full_path)) {
        // Load existing .thm to preserve other settings
        IReader* reader = FS.r_open(root_alias, thm_path.c_str());
        if (reader) {
            // Skip THM_CHUNK_VERSION and THM_CHUNK_DATA (thumbnail)
            if (reader->find_chunk(THM_CHUNK_VERSION)) {
                // Version chunk exists, skip it
            }
            if (reader->find_chunk(THM_CHUNK_DATA)) {
                // Thumbnail data exists, skip it
            }
            // Load texture params
            params.Load(*reader);
            FS.r_close(reader);
            Msg("* [PBRTextureConverter] Loaded existing .thm, updating PBR fields");
        }
    } else {
        Msg("* [PBRTextureConverter] Creating new .thm file");
        // Set default type for new .thm files
        params.type = STextureParams::ttImage;
    }

    // Update PBR texture names
    params.metallic_name = metallic_name.c_str();
    params.roughness_name = roughness_name.c_str();
    params.ao_name = ao_name.c_str();
    params.parallax_name = parallax_name.c_str();

    // Write updated .thm file
    IWriter* writer = FS.w_open(root_alias, thm_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open .thm for writing: %s/%s (full: %s)",
            root_alias, thm_path.c_str(), full_path);
        return false;
    }

    // Write version chunk
    writer->open_chunk(THM_CHUNK_VERSION);
    writer->w_u16(1);  // Version 1
    writer->close_chunk();

    // Write THM_CHUNK_TYPE - required by LoadTHM before params.Load()
    writer->open_chunk(THM_CHUNK_TYPE);
    writer->w_u32(0);  // THM type (0 = texture)
    writer->close_chunk();

    // Write texture params (includes PBR names)
    params.Save(*writer);

    FS.w_close(writer);
    Msg("* [PBRTextureConverter] Successfully wrote .thm: %s", full_path);
    return true;
}

// ══════════════════════════════════════════════════════════
//  CONVERT TEXTURES TO PBR (Phase 2.5.2-2.5.3: Implementation)
// ══════════════════════════════════════════════════════════
// Main conversion loop using xr_parallel_for

bool ConvertTexturesToPBR(
    const TextureInventory& inventory,
    const PBRConversionParams& params,
    PBRConversionStats& out_stats,
    ProgressCallback progress_callback)
{
    if (inventory.assets.empty()) {
        return true;
    }

#ifdef USE_AI_PBR
    // Try to initialize AI pipeline (only once)
    InitializeAIPipeline();
#endif

    const u32 total_textures = static_cast<u32>(inventory.assets.size());

    // Atomic counters for thread-safe statistics
    std::atomic<u32> converted_count{0};
    std::atomic<u32> skipped_count{0};
    std::atomic<u32> failed_count{0};
#ifdef USE_AI_PBR
    std::atomic<u32> ai_conversions{0};
    std::atomic<u32> heuristic_conversions{0};
#endif

    const auto start_time = std::chrono::high_resolution_clock::now();

#ifdef USE_AI_PBR
    Msg("[PBRTextureConverter] Converting %u textures (AI: %s)...",
        total_textures, g_ai_available ? "enabled" : "disabled");
#else
    Msg("[PBRTextureConverter] Converting %u textures (heuristic mode)...", total_textures);
#endif

    // ═══════════════════════════════════════════════════════
    //  SEQUENTIAL CONVERSION LOOP (avoids memory issues)
    // ═══════════════════════════════════════════════════════

    // Process textures sequentially to avoid running out of memory
    // AI models use significant VRAM/RAM, parallel processing causes OOM
    for (size_t idx = 0; idx < inventory.assets.size(); ++idx) {
            const auto& asset = inventory.assets[idx];

            // Show progress every 10 textures or at start
            if (idx % 10 == 0 || idx == 0) {
                Msg("~ [PBRTextureConverter] Processing texture %u/%u: %s",
                    static_cast<u32>(idx + 1),
                    total_textures,
                    asset.base_name.c_str());
            }

            // ═══════════════════════════════════════════════════════
            //  CHECK IF OUTPUTS ALREADY EXIST
            // ═══════════════════════════════════════════════════════
            xr_string albedo_path = asset.base_name + ".dds";           // Replaces original diffuse
            xr_string metallic_path = asset.base_name + "_metallic.dds";
            xr_string roughness_path = asset.base_name + "_roughness.dds";
            xr_string ao_path = asset.base_name + "_ao.dds";
            xr_string parallax_path = asset.base_name + "_parallax.dds";  // Height map

            // PBR texture names (without .dds extension, for .thm reference)
            xr_string metallic_name = asset.base_name + "_metallic";
            xr_string roughness_name = asset.base_name + "_roughness";
            xr_string ao_name = asset.base_name + "_ao";
            xr_string parallax_name = asset.base_name + "_parallax";

            // Check if all PBR textures exist AND .thm is up to date
            bool texturesExist =
                FileExists(params.output_root.c_str(), metallic_path.c_str()) &&
                FileExists(params.output_root.c_str(), roughness_path.c_str()) &&
                FileExists(params.output_root.c_str(), ao_path.c_str()) &&
                FileExists(params.output_root.c_str(), parallax_path.c_str());

            bool thmUpToDate = !THMNeedsPBRUpdate(
                params.output_root.c_str(),
                asset.base_name,
                metallic_name, roughness_name, ao_name, parallax_name);

            if (texturesExist && thmUpToDate) {
                // Already converted and .thm is up to date
                skipped_count++;
                continue;
            }

            // If textures exist but .thm needs update, just update .thm
            if (texturesExist && !thmUpToDate) {
                Msg("* [PBRTextureConverter] PBR textures exist but .thm needs update: %s", asset.base_name.c_str());
                WriteTHMFile(params.output_root.c_str(), asset.base_name,
                            metallic_name, roughness_name, ao_name, parallax_name);
                skipped_count++;  // Count as skipped (no texture conversion needed)
                continue;
            }

            // PBR textures don't exist - will run full conversion
            Msg("* [PBRTextureConverter] PBR textures missing, running conversion: %s", asset.base_name.c_str());

            // ═══════════════════════════════════════════════════════
            //  SKIP IF NO NORMAL MAP (AI requires normal)
            // ═══════════════════════════════════════════════════════
#ifdef USE_AI_PBR
            if (!asset.has_normal) {
                Msg("~ [PBRTextureConverter] Skipping %s (AI requires normal map)", asset.base_name.c_str());
                skipped_count++;
                continue;
            }
#endif

            // ═══════════════════════════════════════════════════════
            //  LOAD SOURCE TEXTURES
            // ═══════════════════════════════════════════════════════
            resources::DDSData diffuseData;
            resources::DDSData specularData;
            resources::DDSData glossData;

            // Helper: Convert VFS path to DDSLoader format (remove .dds extension)
            // DDSLoader::LoadFromFile expects "act\act_arm_2" (no extension)
            // Our relative_path has "act\act_arm_2.dds" (with extension)
            auto StripDDSExtension = [](const xr_string& path) -> xr_string {
                if (path.size() >= 4 && path.substr(path.size() - 4) == ".dds") {
                    return path.substr(0, path.size() - 4);
                }
                return path;
            };

            // Load diffuse (required)
            xr_string diffuse_vfs_path = StripDDSExtension(asset.diffuse.relative_path);
            if (!resources::DDSLoader::LoadFromFile(diffuse_vfs_path.c_str(), diffuseData)) {
                Msg("! [PBRTextureConverter] Failed to load diffuse: %s (VFS path: %s)",
                    asset.diffuse.relative_path.c_str(), diffuse_vfs_path.c_str());
                failed_count++;
                continue;
            }

            // Load specular (optional)
            const resources::DDSData* specularPtr = nullptr;
            if (asset.has_specular) {
                xr_string spec_vfs_path = StripDDSExtension(asset.specular.relative_path);
                if (resources::DDSLoader::LoadFromFile(spec_vfs_path.c_str(), specularData)) {
                    specularPtr = &specularData;
                }
            }

            // Load gloss (optional)
            const resources::DDSData* glossPtr = nullptr;
            if (asset.has_gloss) {
                xr_string gloss_vfs_path = StripDDSExtension(asset.gloss.relative_path);
                if (resources::DDSLoader::LoadFromFile(gloss_vfs_path.c_str(), glossData)) {
                    glossPtr = &glossData;
                }
            }

            // ═══════════════════════════════════════════════════════
            //  CONVERT: Try AI first, fall back to heuristic
            // ═══════════════════════════════════════════════════════
            ConvertedPBRTextures converted;

#ifdef USE_AI_PBR
            // Try AI conversion if available AND we have normal map
            if (g_ai_available && asset.has_normal) {
                // Load normal map
                resources::DDSData normalData;
                xr_string normal_vfs_path = StripDDSExtension(asset.normal.relative_path);

                if (resources::DDSLoader::LoadFromFile(normal_vfs_path.c_str(), normalData)) {
                    converted = ConvertWithAI(&diffuseData, &normalData, params);
                    if (converted.success) {
                        ai_conversions++;
                    }
                }
            }

            // Fall back to heuristic if AI failed or unavailable
            if (!converted.success) {
                converted = ConvertSpecularGlossToPBR(&diffuseData, specularPtr, glossPtr, params);
                if (converted.success) {
                    heuristic_conversions++;
                }
            }
#else
            // Use heuristic conversion (AI not available)
            converted = ConvertSpecularGlossToPBR(&diffuseData, specularPtr, glossPtr, params);
#endif

            if (!converted.success) {
                Msg("! [PBRTextureConverter] Conversion failed: %s", asset.base_name.c_str());
                failed_count++;
                continue;
            }

            // ═══════════════════════════════════════════════════════
            //  WRITE OUTPUT TEXTURES
            // ═══════════════════════════════════════════════════════

            // Write albedo (RGBA - RGB from AI, Alpha preserved from original!)
            if (!converted.albedoData.empty()) {
                if (!WriteRGBADDS(params.output_root.c_str(), albedo_path,
                                  converted.albedoData.data(),
                                  converted.width, converted.height, params.generate_mipmaps)) {
                    Msg("! [PBRTextureConverter] Failed to write albedo: %s/%s",
                        params.output_root.c_str(), albedo_path.c_str());
                    failed_count++;
                    continue;
                }
            }

            // Write metallic (R8)
            if (!WriteSingleChannelDDS(params.output_root.c_str(), metallic_path,
                                       converted.metallicData.data(),
                                       converted.width, converted.height, params.generate_mipmaps)) {
                Msg("! [PBRTextureConverter] Failed to write metallic: %s/%s",
                    params.output_root.c_str(), metallic_path.c_str());
                failed_count++;
                continue;
            }

            // Write roughness (R8)
            if (!WriteSingleChannelDDS(params.output_root.c_str(), roughness_path,
                                       converted.roughnessData.data(),
                                       converted.width, converted.height, params.generate_mipmaps)) {
                Msg("! [PBRTextureConverter] Failed to write roughness: %s/%s",
                    params.output_root.c_str(), roughness_path.c_str());
                failed_count++;
                continue;
            }

            // Write AO (R8)
            if (!converted.aoData.empty()) {
                if (!WriteSingleChannelDDS(params.output_root.c_str(), ao_path,
                                           converted.aoData.data(),
                                           converted.width, converted.height, params.generate_mipmaps)) {
                    Msg("! [PBRTextureConverter] Failed to write AO: %s/%s",
                        params.output_root.c_str(), ao_path.c_str());
                    failed_count++;
                    continue;
                }
            }

            // Write parallax/height (R8)
            if (!converted.parallaxData.empty()) {
                if (!WriteSingleChannelDDS(params.output_root.c_str(), parallax_path,
                                           converted.parallaxData.data(),
                                           converted.width, converted.height, params.generate_mipmaps)) {
                    Msg("! [PBRTextureConverter] Failed to write parallax: %s/%s",
                        params.output_root.c_str(), parallax_path.c_str());
                    failed_count++;
                    continue;
                }
            }

            // ═══════════════════════════════════════════════════════
            //  WRITE .THM FILE WITH PBR TEXTURE REFERENCES
            // ═══════════════════════════════════════════════════════
            WriteTHMFile(params.output_root.c_str(), asset.base_name,
                        metallic_name, roughness_name, ao_name, parallax_name);

            // Success!
            converted_count++;

            // Progress callback
            if (progress_callback) {
                const float progress = static_cast<float>(converted_count + skipped_count + failed_count)
                                     / static_cast<float>(total_textures);
                xr_string status = "Converting ";
                status.append(asset.base_name);
                status.append("...");
                progress_callback(progress, status.c_str());
            }
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Fill statistics
    out_stats.textures_scanned = total_textures;
    out_stats.textures_converted = converted_count.load();
    out_stats.textures_skipped = skipped_count.load();
    out_stats.textures_failed = failed_count.load();
    out_stats.conversion_time_seconds = duration.count() / 1000.0f;

#ifdef USE_AI_PBR
    Msg("[PBRTextureConverter] Conversion complete: %u converted (%u AI, %u heuristic), %u skipped, %u failed (%.2fs)",
        out_stats.textures_converted,
        ai_conversions.load(),
        heuristic_conversions.load(),
        out_stats.textures_skipped,
        out_stats.textures_failed,
        out_stats.conversion_time_seconds);
#else
    Msg("[PBRTextureConverter] Conversion complete: %u converted, %u skipped, %u failed (%.2fs)",
        out_stats.textures_converted,
        out_stats.textures_skipped,
        out_stats.textures_failed,
        out_stats.conversion_time_seconds);
#endif

    return out_stats.textures_failed == 0;
}

} // namespace xray::render::pbr
