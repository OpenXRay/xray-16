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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <DirectXTex.h>  // For normal map generation from diffuse

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
    bool isRGBA8 = (static_cast<int>(ddsData.desc.format) == 19); // R8G8B8A8_UNORM
    bool isR8 = (static_cast<int>(ddsData.desc.format) == 3);     // R8_UNORM (single channel)

    // Handle uncompressed RGBA8 (e.g., generated bumpmaps)
    if (isRGBA8) {
        if (mip0.data && mip0.size >= pixelCount * 4) {
            std::memcpy(result.data(), mip0.data, pixelCount * 4);
        }
        return result;
    }

    // Handle R8_UNORM (single channel) - expand to RGBA
    if (isR8) {
        if (mip0.data && mip0.size >= pixelCount) {
            for (u32 i = 0; i < pixelCount; ++i) {
                u8 r = mip0.data[i];
                result[i * 4 + 0] = r;    // R
                result[i * 4 + 1] = r;    // G (copy R for grayscale view)
                result[i * 4 + 2] = r;    // B (copy R for grayscale view)
                result[i * 4 + 3] = 255;  // A
            }
        }
        return result;
    }

    if (!isDXT1 && !isDXT5) {
        Msg("! [PBRTextureConverter] Unsupported format: %d (only DXT1/DXT5/RGBA8/R8 supported)", static_cast<int>(ddsData.desc.format));
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

// ══════════════════════════════════════════════════════════
//  HELPER: Generate Normal Map from Diffuse using DirectXTex
// ══════════════════════════════════════════════════════════
// Uses luminance-based Sobel filter to create height-derived normal map
// amplitude controls bump strength (higher = more pronounced bumps)

static bool GenerateNormalMapFromDiffuse(
    const xr_vector<u8>& diffuseRGBA,
    u32 width, u32 height,
    xr_vector<u8>& outNormalRGBA,
    float amplitude = 12.0f)
{
    if (diffuseRGBA.empty() || width == 0 || height == 0) {
        return false;
    }

    // Create DirectXTex image from diffuse data
    DirectX::Image srcImage = {};
    srcImage.width = width;
    srcImage.height = height;
    srcImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcImage.rowPitch = width * 4;
    srcImage.slicePitch = width * height * 4;
    srcImage.pixels = const_cast<u8*>(diffuseRGBA.data());

    // Compute normal map using luminance channel as height
    DirectX::ScratchImage normalMap;
    HRESULT hr = DirectX::ComputeNormalMap(
        srcImage,
        DirectX::CNMAP_CHANNEL_LUMINANCE,  // Use luminance as height source
        amplitude,                          // Bump strength
        DXGI_FORMAT_R8G8B8A8_UNORM,
        normalMap
    );

    if (FAILED(hr)) {
        Msg("! [PBRTextureConverter] DirectXTex ComputeNormalMap failed: 0x%08X", hr);
        return false;
    }

    // Copy result to output vector
    const DirectX::Image* result = normalMap.GetImage(0, 0, 0);
    if (!result || !result->pixels) {
        return false;
    }

    // Swizzle from DirectXTex format to X-Ray bump format
    // DirectXTex output: R=X, G=Y, B=Z, A=1 (standard tangent-space normal)
    // X-Ray bump format: R=Gloss, G=Z, B=Y, A=X
    const u32 pixelCount = width * height;
    outNormalRGBA.resize(pixelCount * 4);
    const u8* src = result->pixels;

    // Debug: log first pixel values from DirectXTex
    if (pixelCount > 0) {
        Msg("* [PBRTextureConverter] DirectXTex normal[0]: R=%u G=%u B=%u A=%u",
            src[0], src[1], src[2], src[3]);
    }

    for (u32 i = 0; i < pixelCount; ++i) {
        u8 x = src[i * 4 + 0];  // Normal X from DirectXTex
        u8 y = src[i * 4 + 1];  // Normal Y from DirectXTex
        u8 z = src[i * 4 + 2];  // Normal Z from DirectXTex

        outNormalRGBA[i * 4 + 0] = 128;  // R = Gloss (default 0.5)
        outNormalRGBA[i * 4 + 1] = z;    // G = Normal Z
        outNormalRGBA[i * 4 + 2] = y;    // B = Normal Y
        outNormalRGBA[i * 4 + 3] = x;    // A = Normal X
    }

    // Debug: log first pixel values after swizzle
    if (pixelCount > 0) {
        Msg("* [PBRTextureConverter] X-Ray bump[0]: R=%u G=%u B=%u A=%u (Gloss,Z,Y,X)",
            outNormalRGBA[0], outNormalRGBA[1], outNormalRGBA[2], outNormalRGBA[3]);
    }

    return true;
}

// Helper to write generated normal map as DDS (RGBA8 uncompressed for simplicity)
static bool WriteNormalMapDDS(
    const char* root_alias,
    const xr_string& relative_path,
    const xr_vector<u8>& normalRGBA,
    u32 width, u32 height)
{
    using namespace resources;

    DDS_HEADER header = {};
    header.dwSize = 124;
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_MIPMAPCOUNT;
    header.dwWidth = width;
    header.dwHeight = height;
    header.dwPitchOrLinearSize = width * 4;
    header.dwDepth = 0;
    header.dwMipMapCount = 1;

    // RGBA8 pixel format
    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;  // R in byte 0
    header.ddspf.dwGBitMask = 0x0000FF00;  // G in byte 1
    header.ddspf.dwBBitMask = 0x00FF0000;  // B in byte 2
    header.ddspf.dwABitMask = 0xFF000000;  // A in byte 3

    header.dwCaps = DDSCAPS_TEXTURE;

    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));
    writer->w(&header, sizeof(header));
    writer->w(normalRGBA.data(), normalRGBA.size());

    FS.w_close(writer);
    Msg("* [PBRTextureConverter] Generated normal map: %s/%s (%ux%u)", root_alias, relative_path.c_str(), width, height);
    return true;
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
    "glow",
    "grad",
    "internal",
    "lights",
    "pfx",
    "sky",          // Sky textures
    "ui",           // UI textures
    "intro",
    "terrain",
    "wm",
};

static const char* const METALLIC_WHITELIST[] = {
    "metall", "metal", "iron", "steel", "chrome", "alumin",
    "copper", "brass", "bronze", "gold", "silver", "tin", "zinc",
    "pipe", "rebar", "wire", "chain", "bolt", "nut", "screw",
    "rail", "grate", "mesh",
};

static const char* const NON_METALLIC_KEYWORDS[] = {
    "wood", "bark", "bork",
    "dirt", "ground", "grnd", "soil", "mud", "sand",
    "grass", "leaf", "leaves", "vine", "moss", "bush", "tree",
    "brick", "briks", "concrete", "crete", "stone", "rock", "gravel",
    "plaster", "stucco", "cement", "asphalt", "road",
    "cloth", "fabric", "carpet", "curtain", "leather",
    "paper", "cardboard",
    "skin", "flesh", "body",
    "food",
    "oblaka", "sky", "cloud",
    "water",
    "detail", "build_details",
    "floor", "roof", "tile", "wall",
};

static bool IsNonMetallic(const xr_string& name) {
    xr_string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const char* keyword : METALLIC_WHITELIST) {
        if (lower.find(keyword) != xr_string::npos)
            return false;
    }
    for (const char* keyword : NON_METALLIC_KEYWORDS) {
        if (lower.find(keyword) != xr_string::npos)
            return true;
    }
    return false;
}

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

static bool ReadDDSDimensions(const char* root_alias, const char* relative_path, u32& outWidth, u32& outHeight) {
    string_path full_path;
    FS.update_path(full_path, root_alias, relative_path);
    if (!FS.exist(full_path))
        return false;

    IReader* reader = FS.r_open(root_alias, relative_path);
    if (!reader) return false;

    if (reader->length() < 20) {
        FS.r_close(reader);
        return false;
    }

    u32 magic = reader->r_u32();
    if (magic != 0x20534444) {
        FS.r_close(reader);
        return false;
    }

    reader->r_u32();
    reader->r_u32();
    outHeight = reader->r_u32();
    outWidth  = reader->r_u32();

    FS.r_close(reader);
    return (outWidth > 0 && outHeight > 0);
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
                file_name.find("_pbr") != xr_string::npos ||    // Skip consolidated PBR textures
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

            ReadDDSDimensions(root.c_str(), file_name.c_str(), asset.diffuse_width, asset.diffuse_height);

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

// Forward declaration for WriteTHMFileConsolidated (defined in consolidation section)
static bool WriteTHMFileConsolidated(const char* root_alias, const xr_string& base_name, const xr_string& pbr_name);

// ══════════════════════════════════════════════════════════
//  HELPER: Check if .thm needs PBR update
// ══════════════════════════════════════════════════════════
// Returns true if .thm doesn't exist or lacks PBR texture references

static bool THMNeedsPBRUpdate(
    const char* root_alias,
    const xr_string& base_name,
    const xr_string& pbr_name)  // Check for consolidated pbr_name
{
    xr_string thm_path = base_name + ".thm";

    string_path full_path;
    FS.update_path(full_path, root_alias, thm_path.c_str());

    if (!FS.exist(full_path)) {
        return true;  // .thm doesn't exist
    }

    // Load existing .thm and check PBR fields
    IReader* reader = FS.r_open(root_alias, thm_path.c_str());
    if (!reader) {
        return true;  // Can't read, needs update
    }

    STextureParams params;
    params.Clear();
    params.Load(*reader);
    FS.r_close(reader);

    // Check if consolidated pbr_name matches expected value
    if (params.pbr_name != pbr_name.c_str()) {
        return true;  // pbr_name doesn't match
    }

    return false;  // .thm is up to date with consolidated PBR
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
        // Check if consolidated _pbr.dds exists (preferred)
        xr_string pbr_path = asset.base_name + "_pbr.dds";
        xr_string pbr_name = asset.base_name + "_pbr";

        if (FileExists(params.output_root.c_str(), pbr_path.c_str())) {
            // Consolidated PBR exists - check if .thm has pbr_name field
            if (THMNeedsPBRUpdate(params.output_root.c_str(), asset.base_name, pbr_name)) {
                missing_thm++;
            }
        } else {
            // No consolidated _pbr.dds - need conversion
            missing_textures++;
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

static void ZeroMetallicIfBlacklisted(ConvertedPBRTextures& converted, const xr_string& name) {
    if (!converted.success || converted.metallicData.empty())
        return;
    if (IsNonMetallic(name)) {
        std::memset(converted.metallicData.data(), 0, converted.metallicData.size());
        Msg("~ [PBRTextureConverter] Forced metallic=0 for non-metallic material: %s", name.c_str());
    }
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

    // UNet models require minimum texture size due to encoder downsampling
    // 4 downsampling stages (stride 2 each) means minimum 32x32 (2^5)
    constexpr u32 MIN_AI_TEXTURE_SIZE = 32;
    if (diffuseWidth < MIN_AI_TEXTURE_SIZE || diffuseHeight < MIN_AI_TEXTURE_SIZE) {
        // Upscale small textures to minimum size
        u32 newWidth = std::max(diffuseWidth, MIN_AI_TEXTURE_SIZE);
        u32 newHeight = std::max(diffuseHeight, MIN_AI_TEXTURE_SIZE);
        Msg("~ [PBRTextureConverter] Upscaling small texture %ux%u → %ux%u for AI processing",
            diffuseWidth, diffuseHeight, newWidth, newHeight);

        // Upscale diffuse with nearest-neighbor (preserves pixel art style)
        xr_vector<u8> upscaledDiffuse(newWidth * newHeight * 4);
        for (u32 y = 0; y < newHeight; ++y) {
            for (u32 x = 0; x < newWidth; ++x) {
                u32 srcX = x * diffuseWidth / newWidth;
                u32 srcY = y * diffuseHeight / newHeight;
                u32 srcIdx = (srcY * diffuseWidth + srcX) * 4;
                u32 dstIdx = (y * newWidth + x) * 4;
                upscaledDiffuse[dstIdx + 0] = diffuseRGB[srcIdx + 0];
                upscaledDiffuse[dstIdx + 1] = diffuseRGB[srcIdx + 1];
                upscaledDiffuse[dstIdx + 2] = diffuseRGB[srcIdx + 2];
                upscaledDiffuse[dstIdx + 3] = diffuseRGB[srcIdx + 3];
            }
        }
        diffuseRGB = std::move(upscaledDiffuse);

        // Upscale normal with nearest-neighbor
        xr_vector<u8> upscaledNormal(newWidth * newHeight * 4);
        for (u32 y = 0; y < newHeight; ++y) {
            for (u32 x = 0; x < newWidth; ++x) {
                u32 srcX = x * normalWidth / newWidth;
                u32 srcY = y * normalHeight / newHeight;
                u32 srcIdx = (srcY * normalWidth + srcX) * 4;
                u32 dstIdx = (y * newWidth + x) * 4;
                upscaledNormal[dstIdx + 0] = normalRGB[srcIdx + 0];
                upscaledNormal[dstIdx + 1] = normalRGB[srcIdx + 1];
                upscaledNormal[dstIdx + 2] = normalRGB[srcIdx + 2];
                upscaledNormal[dstIdx + 3] = normalRGB[srcIdx + 3];
            }
        }
        normalRGB = std::move(upscaledNormal);

        diffuseWidth = newWidth;
        diffuseHeight = newHeight;
        normalWidth = newWidth;
        normalHeight = newHeight;
    }

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

static xr_vector<u8> NearestNeighborUpscaleRGBA(
    const xr_vector<u8>& src, u32 srcW, u32 srcH, u32 dstW, u32 dstH)
{
    xr_vector<u8> dst(dstW * dstH * 4);
    for (u32 y = 0; y < dstH; ++y) {
        for (u32 x = 0; x < dstW; ++x) {
            u32 srcIdx = (y * srcH / dstH * srcW + x * srcW / dstW) * 4;
            u32 dstIdx = (y * dstW + x) * 4;
            dst[dstIdx + 0] = src[srcIdx + 0];
            dst[dstIdx + 1] = src[srcIdx + 1];
            dst[dstIdx + 2] = src[srcIdx + 2];
            dst[dstIdx + 3] = src[srcIdx + 3];
        }
    }
    return dst;
}

struct DecompressedPair {
    xr_vector<u8> diffuseRGBA;
    xr_vector<u8> normalRGBA;
    u32 width = 0;
    u32 height = 0;
    bool success = false;
};

static DecompressedPair DecompressAndPrepare(
    const resources::DDSData* diffuseData,
    const resources::DDSData* normalData)
{
    DecompressedPair result;

    if (!diffuseData || !diffuseData->isValid || diffuseData->mipLevels.empty())
        return result;
    if (!normalData || !normalData->isValid || normalData->mipLevels.empty())
        return result;

    u32 diffuseWidth, diffuseHeight, normalWidth, normalHeight;
    result.diffuseRGBA = DecompressDDS(*diffuseData, diffuseWidth, diffuseHeight);
    result.normalRGBA = DecompressDDS(*normalData, normalWidth, normalHeight);

    if (result.diffuseRGBA.empty() || result.normalRGBA.empty()) {
        Msg("! [PBRTextureConverter] Failed to decompress textures");
        return result;
    }

    Msg("~ [PBRTextureConverter] Decompressed dimensions: diffuse %ux%u, normal %ux%u",
        diffuseWidth, diffuseHeight, normalWidth, normalHeight);

    constexpr u32 MIN_AI_TEXTURE_SIZE = 32;
    if (diffuseWidth < MIN_AI_TEXTURE_SIZE || diffuseHeight < MIN_AI_TEXTURE_SIZE) {
        u32 newWidth = std::max(diffuseWidth, MIN_AI_TEXTURE_SIZE);
        u32 newHeight = std::max(diffuseHeight, MIN_AI_TEXTURE_SIZE);
        Msg("~ [PBRTextureConverter] Upscaling small texture %ux%u → %ux%u for AI processing",
            diffuseWidth, diffuseHeight, newWidth, newHeight);

        result.diffuseRGBA = NearestNeighborUpscaleRGBA(result.diffuseRGBA, diffuseWidth, diffuseHeight, newWidth, newHeight);
        result.normalRGBA = NearestNeighborUpscaleRGBA(result.normalRGBA, normalWidth, normalHeight, newWidth, newHeight);

        diffuseWidth = newWidth;
        diffuseHeight = newHeight;
        normalWidth = newWidth;
        normalHeight = newHeight;
    }

    if (normalWidth != diffuseWidth || normalHeight != diffuseHeight) {
        Msg("~ [PBRTextureConverter] Upscaling normal map: %ux%u → %ux%u",
            normalWidth, normalHeight, diffuseWidth, diffuseHeight);
        result.normalRGBA = NearestNeighborUpscaleRGBA(result.normalRGBA, normalWidth, normalHeight, diffuseWidth, diffuseHeight);
    }

    result.width = diffuseWidth;
    result.height = diffuseHeight;
    result.success = true;
    return result;
}

static ConvertedPBRTextures PackOutputs(PBRPipelineOutputs& outputs, const xr_vector<u8>& originalDiffuseRGBA, u32 width, u32 height) {
    ConvertedPBRTextures result;
    result.width = width;
    result.height = height;

    xr_vector<u8> aiAlbedoRGB = outputs.albedo.ToImageData(false);
    result.metallicData = outputs.metallic.ToImageData(true);
    result.roughnessData = outputs.roughness.ToImageData(true);
    result.aoData = outputs.ao.ToImageData(true);
    result.parallaxData = outputs.parallax.ToImageData(true);

    const u32 pixelCount = width * height;
    result.albedoData.resize(pixelCount * 4);

    for (u32 i = 0; i < pixelCount; ++i) {
        result.albedoData[i * 4 + 0] = aiAlbedoRGB[i * 3 + 0];
        result.albedoData[i * 4 + 1] = aiAlbedoRGB[i * 3 + 1];
        result.albedoData[i * 4 + 2] = aiAlbedoRGB[i * 3 + 2];
        result.albedoData[i * 4 + 3] = originalDiffuseRGBA[i * 4 + 3];
    }

    result.success = true;
    return result;
}

struct Stage1Intermediate {
    PBRPipeline::Stage1Result stage1;
    xr_vector<u8> originalDiffuseRGBA;
    u32 width = 0;
    u32 height = 0;
    xr_string base_name;
    xr_string pbr_path;
    xr_string pbr_name;
    xr_string albedo_path;
};

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

// ══════════════════════════════════════════════════════════
//  HELPER: Write Packed PBR DDS (Metallic/Roughness/AO/Parallax)
// ══════════════════════════════════════════════════════════
// Packs 4 single-channel textures into one RGBA8 texture:
//   R = Metallic
//   G = Roughness
//   B = AO
//   A = Parallax (height map, or 255 if not provided)

static bool WritePackedPBRDDS(
    const char* root_alias,
    const xr_string& relative_path,
    const u8* metallicData,
    const u8* roughnessData,
    const u8* aoData,
    const u8* parallaxData,  // Can be nullptr
    u32 width,
    u32 height,
    bool generateMipmaps)
{
    // Pack into RGBA8
    const u32 pixelCount = width * height;
    xr_vector<u8> packedData(pixelCount * 4);

    for (u32 i = 0; i < pixelCount; ++i) {
        packedData[i * 4 + 0] = metallicData ? metallicData[i] : 0;      // R = Metallic
        packedData[i * 4 + 1] = roughnessData ? roughnessData[i] : 128;  // G = Roughness (default 0.5)
        packedData[i * 4 + 2] = aoData ? aoData[i] : 255;                // B = AO (default 1.0)
        packedData[i * 4 + 3] = parallaxData ? parallaxData[i] : 128;    // A = Parallax (default 0.5)
    }

    return WriteRGBADDS(root_alias, relative_path, packedData.data(), width, height, generateMipmaps);
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

template<typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t max_size) : max_size_(max_size) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this] { return queue_.size() < max_size_ || done_; });
        if (done_) return;
        queue_.push(std::move(item));
        not_empty_.notify_one();
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return !queue_.empty() || done_; });
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return true;
    }

    void signal_done() {
        std::unique_lock<std::mutex> lock(mutex_);
        done_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t max_size_;
    bool done_ = false;
};

struct PreparedTexture {
    xr_string base_name;
    xr_string pbr_path;
    xr_string pbr_name;
    xr_string albedo_path;
    resources::DDSData diffuseData;
    resources::DDSData normalData;
};

struct ConvertedOutput {
    xr_string base_name;
    xr_string pbr_path;
    xr_string pbr_name;
    xr_string albedo_path;
    ConvertedPBRTextures converted;
    bool generate_mipmaps;
    xr_string output_root;
};

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
    InitializeAIPipeline();
#endif

    const u32 total_textures = static_cast<u32>(inventory.assets.size());

    std::atomic<u32> converted_count{0};
    std::atomic<u32> skipped_count{0};
    std::atomic<u32> failed_count{0};
#ifdef USE_AI_PBR
    std::atomic<u32> ai_conversions{0};
#endif

    const auto start_time = std::chrono::high_resolution_clock::now();

    xr_vector<LegacyTextureAsset> sorted_assets(inventory.assets.begin(), inventory.assets.end());
    std::sort(sorted_assets.begin(), sorted_assets.end(),
        [](const LegacyTextureAsset& a, const LegacyTextureAsset& b) {
            if (a.diffuse_width != b.diffuse_width)
                return a.diffuse_width < b.diffuse_width;
            if (a.diffuse_height != b.diffuse_height)
                return a.diffuse_height < b.diffuse_height;
            return a.diffuse.file_size < b.diffuse.file_size;
        });

    Msg("[PBRTextureConverter] Converting %u textures (pipelined, sorted by dimensions)...", total_textures);

#ifndef USE_AI_PBR
    Msg("! [PBRTextureConverter] USE_AI_PBR not defined, cannot convert any textures");
    out_stats.textures_scanned = total_textures;
    out_stats.textures_skipped = total_textures;
    return true;
#else
    if (!g_ai_available) {
        Msg("! [PBRTextureConverter] AI pipeline not available, cannot convert any textures");
        out_stats.textures_scanned = total_textures;
        out_stats.textures_skipped = total_textures;
        return true;
    }

    {
        xr_vector<std::pair<u32, u32>> unique_dims;
        for (const auto& asset : sorted_assets) {
            if (asset.diffuse_width == 0 || asset.diffuse_height == 0)
                continue;
            std::pair<u32, u32> dim = { asset.diffuse_width, asset.diffuse_height };
            if (unique_dims.empty() || unique_dims.back() != dim)
                unique_dims.push_back(dim);
        }

        if (!unique_dims.empty()) {
            ScopeLock lock{ &g_ai_pipeline_mutex };
            g_ai_pipeline->WarmupTRTEngines(unique_dims);
        }
    }

    BoundedQueue<PreparedTexture> prepared_queue(2);
    BoundedQueue<ConvertedOutput> write_queue(2);

    auto StripDDSExtension = [](const xr_string& path) -> xr_string {
        if (path.size() >= 4 && path.substr(path.size() - 4) == ".dds") {
            return path.substr(0, path.size() - 4);
        }
        return path;
    };

    std::thread producer([&]() {
        for (size_t idx = 0; idx < sorted_assets.size(); ++idx) {
            LegacyTextureAsset asset = sorted_assets[idx];

            if (idx % 10 == 0 || idx == 0) {
                Msg("~ [PBRTextureConverter] Preparing texture %u/%u: %s",
                    static_cast<u32>(idx + 1), total_textures, asset.base_name.c_str());
            }

            xr_string pbr_path = asset.base_name + "_pbr.dds";
            xr_string pbr_name = asset.base_name + "_pbr";

            bool pbrExists = FileExists(params.output_root.c_str(), pbr_path.c_str());
            bool thmUpToDate = !THMNeedsPBRUpdate(params.output_root.c_str(), asset.base_name, pbr_name);

            if (pbrExists && thmUpToDate) {
                skipped_count++;
                continue;
            }

            if (pbrExists && !thmUpToDate) {
                WriteTHMFileConsolidated(params.output_root.c_str(), asset.base_name, pbr_name);
                skipped_count++;
                continue;
            }

            resources::DDSData diffuseData;
            xr_string diffuse_vfs_path = StripDDSExtension(asset.diffuse.relative_path);
            if (!resources::DDSLoader::LoadFromFile(diffuse_vfs_path.c_str(), diffuseData)) {
                Msg("! [PBRTextureConverter] Failed to load diffuse: %s", asset.diffuse.relative_path.c_str());
                failed_count++;
                continue;
            }

            if (!asset.has_normal) {
                Msg("~ [PBRTextureConverter] No normal map for %s, generating from diffuse...", asset.base_name.c_str());
                u32 diffuseWidth, diffuseHeight;
                xr_vector<u8> diffuseRGBA = DecompressDDS(diffuseData, diffuseWidth, diffuseHeight);

                if (!diffuseRGBA.empty()) {
                    xr_vector<u8> generatedNormalRGBA;
                    if (GenerateNormalMapFromDiffuse(diffuseRGBA, diffuseWidth, diffuseHeight, generatedNormalRGBA, 4.0f)) {
                        xr_string bump_path = asset.base_name + "_bump.dds";
                        if (WriteNormalMapDDS(params.output_root.c_str(), bump_path, generatedNormalRGBA, diffuseWidth, diffuseHeight)) {
                            asset.has_normal = true;
                            asset.normal.root_alias = params.output_root;
                            asset.normal.relative_path = bump_path;
                            Msg("* [PBRTextureConverter] Generated normal map saved: %s", bump_path.c_str());
                        }
                    }
                }
            }

            if (!asset.has_normal) {
                Msg("~ [PBRTextureConverter] Skipping %s (no normal map available)", asset.base_name.c_str());
                skipped_count++;
                continue;
            }

            resources::DDSData normalData;
            xr_string normal_vfs_path = StripDDSExtension(asset.normal.relative_path);
            if (!resources::DDSLoader::LoadFromFile(normal_vfs_path.c_str(), normalData)) {
                Msg("! [PBRTextureConverter] Failed to load normal: %s", asset.normal.relative_path.c_str());
                failed_count++;
                continue;
            }

            Msg("* [PBRTextureConverter] PBR textures missing, running conversion: %s", asset.base_name.c_str());

            PreparedTexture prepared;
            prepared.base_name = asset.base_name;
            prepared.pbr_path = std::move(pbr_path);
            prepared.pbr_name = std::move(pbr_name);
            prepared.albedo_path = asset.base_name + ".dds";
            prepared.diffuseData = std::move(diffuseData);
            prepared.normalData = std::move(normalData);
            prepared_queue.push(std::move(prepared));
        }
        prepared_queue.signal_done();
    });

    std::thread writer([&]() {
        ConvertedOutput output;
        while (write_queue.pop(output)) {
            auto t_write_start = std::chrono::high_resolution_clock::now();

            if (!output.converted.albedoData.empty()) {
                if (!WriteRGBADDS(output.output_root.c_str(), output.albedo_path,
                                  output.converted.albedoData.data(),
                                  output.converted.width, output.converted.height, output.generate_mipmaps)) {
                    Msg("! [PBRTextureConverter] Failed to write albedo: %s", output.albedo_path.c_str());
                    failed_count++;
                    continue;
                }
            }

            if (!WritePackedPBRDDS(output.output_root.c_str(), output.pbr_path,
                                   output.converted.metallicData.data(),
                                   output.converted.roughnessData.data(),
                                   output.converted.aoData.empty() ? nullptr : output.converted.aoData.data(),
                                   output.converted.parallaxData.empty() ? nullptr : output.converted.parallaxData.data(),
                                   output.converted.width, output.converted.height, output.generate_mipmaps)) {
                Msg("! [PBRTextureConverter] Failed to write packed PBR: %s", output.pbr_path.c_str());
                failed_count++;
                continue;
            }

            WriteTHMFileConsolidated(output.output_root.c_str(), output.base_name, output.pbr_name);

            auto t_write_end = std::chrono::high_resolution_clock::now();
            auto write_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_write_end - t_write_start).count();
            Msg("* [PBRTextureConverter] Consolidated PBR: %s (%ux%u) [write %lldms]",
                output.pbr_path.c_str(), output.converted.width, output.converted.height, write_ms);

            converted_count++;

            if (progress_callback) {
                const float progress = static_cast<float>(converted_count + skipped_count + failed_count)
                                     / static_cast<float>(total_textures);
                xr_string status = "Converting ";
                status.append(output.base_name);
                status.append("...");
                progress_callback(progress, status.c_str());
            }
        }
    });

    {
        auto EmitResult = [&](Stage1Intermediate& inter, ConvertedPBRTextures&& converted, long long infer_ms) {
            if (!converted.success) {
                Msg("! [PBRTextureConverter] Conversion failed: %s [%lldms]", inter.base_name.c_str(), infer_ms);
                failed_count++;
                return;
            }

            Msg("~ [PBRTextureConverter] AI inference done: %s (%ux%u) [%lldms]",
                inter.base_name.c_str(), converted.width, converted.height, infer_ms);

            ZeroMetallicIfBlacklisted(converted, inter.base_name);
            ai_conversions++;

            ConvertedOutput output;
            output.base_name = std::move(inter.base_name);
            output.pbr_path = std::move(inter.pbr_path);
            output.pbr_name = std::move(inter.pbr_name);
            output.albedo_path = std::move(inter.albedo_path);
            output.converted = std::move(converted);
            output.generate_mipmaps = params.generate_mipmaps;
            output.output_root = params.output_root;
            write_queue.push(std::move(output));
        };

        auto FlushLargeBatch = [&](xr_vector<Stage1Intermediate>& batch) {
            if (batch.empty()) return;

            Msg("[PBRTextureConverter] Stage2 pass for %zu large textures (%ux%u)",
                batch.size(), batch[0].width, batch[0].height);

            for (auto& inter : batch) {
                auto t_start = std::chrono::high_resolution_clock::now();
                PBRPipelineOutputs outputs = g_ai_pipeline->ProcessStage2(inter.stage1);
                auto t_end = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

                ConvertedPBRTextures converted;
                if (outputs.success)
                    converted = PackOutputs(outputs, inter.originalDiffuseRGBA, inter.width, inter.height);

                EmitResult(inter, std::move(converted), ms);
            }
            batch.clear();
        };

        xr_vector<Stage1Intermediate> large_batch;
        PreparedTexture prepared;

        while (prepared_queue.pop(prepared)) {
            auto t_infer_start = std::chrono::high_resolution_clock::now();

            DecompressedPair decompressed = DecompressAndPrepare(&prepared.diffuseData, &prepared.normalData);
            if (!decompressed.success) {
                Msg("! [PBRTextureConverter] Decompress failed: %s", prepared.base_name.c_str());
                failed_count++;
                continue;
            }

            Msg("~ [PBRTextureConverter] Calling Process with dimensions: width=%u, height=%u",
                decompressed.width, decompressed.height);

            const bool large = g_ai_pipeline->NeedsSplitProcessing(decompressed.width, decompressed.height);

            if (!large_batch.empty() &&
                (large_batch[0].width != decompressed.width || large_batch[0].height != decompressed.height)) {
                FlushLargeBatch(large_batch);
            }

            if (!large) {
                PBRPipelineOutputs outputs = g_ai_pipeline->Process(
                    decompressed.diffuseRGBA.data(),
                    decompressed.normalRGBA.data(),
                    decompressed.width,
                    decompressed.height
                );

                auto t_infer_end = std::chrono::high_resolution_clock::now();
                auto infer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_infer_end - t_infer_start).count();

                ConvertedPBRTextures converted;
                if (outputs.success)
                    converted = PackOutputs(outputs, decompressed.diffuseRGBA, decompressed.width, decompressed.height);

                Stage1Intermediate dummy;
                dummy.base_name = std::move(prepared.base_name);
                dummy.pbr_path = std::move(prepared.pbr_path);
                dummy.pbr_name = std::move(prepared.pbr_name);
                dummy.albedo_path = std::move(prepared.albedo_path);
                EmitResult(dummy, std::move(converted), infer_ms);
            } else {
                PBRPipeline::Stage1Result stage1 = g_ai_pipeline->ProcessStage1(
                    decompressed.diffuseRGBA.data(),
                    decompressed.normalRGBA.data(),
                    decompressed.width,
                    decompressed.height
                );

                if (!stage1.success) {
                    auto t_infer_end = std::chrono::high_resolution_clock::now();
                    auto infer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_infer_end - t_infer_start).count();
                    Msg("! [PBRTextureConverter] Stage1 failed: %s [%lldms]", prepared.base_name.c_str(), infer_ms);
                    failed_count++;
                    continue;
                }

                Stage1Intermediate inter;
                inter.stage1 = std::move(stage1);
                inter.originalDiffuseRGBA = std::move(decompressed.diffuseRGBA);
                inter.width = decompressed.width;
                inter.height = decompressed.height;
                inter.base_name = std::move(prepared.base_name);
                inter.pbr_path = std::move(prepared.pbr_path);
                inter.pbr_name = std::move(prepared.pbr_name);
                inter.albedo_path = std::move(prepared.albedo_path);
                large_batch.push_back(std::move(inter));
            }
        }

        FlushLargeBatch(large_batch);

        write_queue.signal_done();
    }

    producer.join();
    writer.join();
#endif

    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    out_stats.textures_scanned = total_textures;
    out_stats.textures_converted = converted_count.load();
    out_stats.textures_skipped = skipped_count.load();
    out_stats.textures_failed = failed_count.load();
    out_stats.conversion_time_seconds = duration.count() / 1000.0f;

    Msg("[PBRTextureConverter] Conversion complete: %u converted, %u skipped, %u failed (%.2fs)",
        out_stats.textures_converted,
        out_stats.textures_skipped,
        out_stats.textures_failed,
        out_stats.conversion_time_seconds);

    return out_stats.textures_failed == 0;
}

// ══════════════════════════════════════════════════════════
//  CONSOLIDATE PBR TEXTURES (Pack separate files into _pbr.dds)
// ══════════════════════════════════════════════════════════
// Scans for existing _metallic.dds, _roughness.dds, _ao.dds, _parallax.dds
// Packs them into a single _pbr.dds (R=metallic, G=roughness, B=ao, A=parallax)
// Updates .thm to use pbr_name field, deletes old separate files

// Helper to load single-channel DDS into byte array
static xr_vector<u8> LoadSingleChannelDDS(const char* root_alias, const xr_string& path, u32& outWidth, u32& outHeight)
{
    xr_vector<u8> result;
    outWidth = outHeight = 0;

    // Strip .dds extension for DDSLoader
    xr_string vfs_path = path;
    if (vfs_path.size() >= 4 && vfs_path.substr(vfs_path.size() - 4) == ".dds") {
        vfs_path = vfs_path.substr(0, vfs_path.size() - 4);
    }

    resources::DDSData ddsData;
    if (!resources::DDSLoader::LoadFromFile(vfs_path.c_str(), ddsData)) {
        return result;
    }

    // Decompress to RGBA
    xr_vector<u8> rgba = DecompressDDS(ddsData, outWidth, outHeight);
    if (rgba.empty()) {
        return result;
    }

    // Extract single channel (R) - our single-channel DDS uses R8 or RGBA with data in R
    const u32 pixelCount = outWidth * outHeight;
    result.resize(pixelCount);

    // Check if it's actually single channel or RGBA
    if (rgba.size() == pixelCount) {
        // Already single channel
        result = std::move(rgba);
    } else if (rgba.size() == pixelCount * 4) {
        // RGBA - extract R channel
        for (u32 i = 0; i < pixelCount; ++i) {
            result[i] = rgba[i * 4];  // R channel
        }
    }

    return result;
}

// Helper to update .thm with consolidated pbr_name
static bool WriteTHMFileConsolidated(
    const char* root_alias,
    const xr_string& base_name,
    const xr_string& pbr_name)
{
    xr_string thm_path = base_name + ".thm";

    STextureParams params;
    params.Clear();

    string_path full_path;
    FS.update_path(full_path, root_alias, thm_path.c_str());

    if (FS.exist(full_path)) {
        IReader* reader = FS.r_open(root_alias, thm_path.c_str());
        if (reader) {
            if (reader->find_chunk(THM_CHUNK_VERSION)) {}
            if (reader->find_chunk(THM_CHUNK_DATA)) {}
            params.Load(*reader);
            FS.r_close(reader);
        }
    } else {
        params.type = STextureParams::ttImage;
    }

    // Clear old separate PBR fields, set consolidated pbr_name
    params.metallic_name = "";
    params.roughness_name = "";
    params.ao_name = "";
    params.parallax_name = "";
    params.pbr_name = pbr_name.c_str();

    IWriter* writer = FS.w_open(root_alias, thm_path.c_str());
    if (!writer) {
        return false;
    }

    writer->open_chunk(THM_CHUNK_VERSION);
    writer->w_u16(1);
    writer->close_chunk();

    writer->open_chunk(THM_CHUNK_TYPE);
    writer->w_u32(0);
    writer->close_chunk();

    params.Save(*writer);
    FS.w_close(writer);

    return true;
}

// Helper to delete a file
static bool DeleteFile(const char* root_alias, const xr_string& path)
{
    string_path full_path;
    FS.update_path(full_path, root_alias, path.c_str());

    if (!FS.exist(full_path)) {
        return true;  // Already gone
    }

    // Use standard file deletion
    return (std::remove(full_path) == 0);
}

bool ConsolidatePBRTextures(
    const xr_string& root_alias,
    ConsolidationStats& out_stats,
    ProgressCallback progress_callback)
{
    Msg("[PBRTextureConverter] Starting PBR texture consolidation...");

    // Scan for _metallic.dds files (indicates PBR textures exist)
    FS_FileSet metallic_files;
    FS.file_list(metallic_files, root_alias.c_str(), FS_ListFiles, "*_metallic.dds");

    out_stats.textures_found = static_cast<u32>(metallic_files.size());
    Msg("[PBRTextureConverter] Found %u texture sets to consolidate", out_stats.textures_found);

    if (metallic_files.empty()) {
        return true;
    }

    u32 processed = 0;
    for (const auto& file : metallic_files) {
        // Extract base name from metallic path
        xr_string metallic_path = file.name.c_str();
        xr_string base_name = metallic_path.substr(0, metallic_path.size() - 13);  // Remove "_metallic.dds"

        // Build paths for all PBR textures
        xr_string roughness_path = base_name + "_roughness.dds";
        xr_string ao_path = base_name + "_ao.dds";
        xr_string parallax_path = base_name + "_parallax.dds";
        xr_string pbr_path = base_name + "_pbr.dds";

        // Check if already consolidated
        string_path pbr_full_path;
        FS.update_path(pbr_full_path, root_alias.c_str(), pbr_path.c_str());
        if (FS.exist(pbr_full_path)) {
            processed++;
            continue;  // Already done
        }

        // Load existing separate textures
        u32 width = 0, height = 0;
        u32 tempW, tempH;

        xr_vector<u8> metallicData = LoadSingleChannelDDS(root_alias.c_str(), metallic_path, width, height);
        if (metallicData.empty()) {
            Msg("! [PBRTextureConverter] Failed to load metallic: %s", metallic_path.c_str());
            out_stats.textures_failed++;
            continue;
        }

        xr_vector<u8> roughnessData = LoadSingleChannelDDS(root_alias.c_str(), roughness_path, tempW, tempH);
        if (roughnessData.empty() || tempW != width || tempH != height) {
            Msg("! [PBRTextureConverter] Failed to load roughness or size mismatch: %s", roughness_path.c_str());
            out_stats.textures_failed++;
            continue;
        }

        xr_vector<u8> aoData = LoadSingleChannelDDS(root_alias.c_str(), ao_path, tempW, tempH);
        if (aoData.empty() || tempW != width || tempH != height) {
            Msg("! [PBRTextureConverter] Failed to load AO or size mismatch: %s", ao_path.c_str());
            out_stats.textures_failed++;
            continue;
        }

        // Parallax is optional
        xr_vector<u8> parallaxData = LoadSingleChannelDDS(root_alias.c_str(), parallax_path, tempW, tempH);
        if (!parallaxData.empty() && (tempW != width || tempH != height)) {
            parallaxData.clear();  // Size mismatch, skip parallax
        }

        // Pack and write consolidated _pbr.dds
        if (!WritePackedPBRDDS(root_alias.c_str(), pbr_path,
                               metallicData.data(),
                               roughnessData.data(),
                               aoData.data(),
                               parallaxData.empty() ? nullptr : parallaxData.data(),
                               width, height, true)) {
            Msg("! [PBRTextureConverter] Failed to write packed PBR: %s", pbr_path.c_str());
            out_stats.textures_failed++;
            continue;
        }

        Msg("* [PBRTextureConverter] Consolidated: %s (%ux%u)", pbr_path.c_str(), width, height);

        // Update .thm to use pbr_name
        xr_string pbr_name = base_name + "_pbr";
        WriteTHMFileConsolidated(root_alias.c_str(), base_name, pbr_name);

        // Delete old separate files
        if (DeleteFile(root_alias.c_str(), metallic_path)) out_stats.files_deleted++;
        if (DeleteFile(root_alias.c_str(), roughness_path)) out_stats.files_deleted++;
        if (DeleteFile(root_alias.c_str(), ao_path)) out_stats.files_deleted++;
        if (!parallaxData.empty() && DeleteFile(root_alias.c_str(), parallax_path)) out_stats.files_deleted++;

        out_stats.textures_consolidated++;
        processed++;

        // Progress callback
        if (progress_callback) {
            float progress = static_cast<float>(processed) / static_cast<float>(out_stats.textures_found);
            xr_string status = "Consolidating " + base_name + "...";
            progress_callback(progress, status.c_str());
        }
    }

    Msg("[PBRTextureConverter] Consolidation complete: %u consolidated, %u failed, %u files deleted",
        out_stats.textures_consolidated, out_stats.textures_failed, out_stats.files_deleted);

    return out_stats.textures_failed == 0;
}

bool ConvertSingleTextureToPBR(
    const char* root_alias,
    const char* relative_path,
    const PBRConversionParams& params)
{
    if (!FileExists(root_alias, relative_path))
    {
        Msg("! [PBRTextureConverter] Texture not found: %s/%s", root_alias, relative_path);
        return false;
    }

    xr_string rel(relative_path);
    xr_string base_name = rel;
    if (const auto dot = base_name.find_last_of('.'); dot != xr_string::npos)
        base_name = base_name.substr(0, dot);

    xr_string pbr_path = base_name + "_pbr.dds";
    if (FileExists(root_alias, pbr_path.c_str()))
    {
        Msg("~ [PBRTextureConverter] %s already exists, skipping", pbr_path.c_str());
        return true;
    }

    LegacyTextureAsset asset;
    asset.base_name = base_name;
    asset.diffuse.root_alias = root_alias;
    asset.diffuse.relative_path = relative_path;

    TextureInventory inventory;
    inventory.assets.push_back(std::move(asset));
    inventory.total_textures = 1;

    PBRConversionParams localParams = params;
    localParams.output_root = root_alias;

    PBRConversionStats stats;
    if (!ConvertTexturesToPBR(inventory, localParams, stats, nullptr))
    {
        Msg("! [PBRTextureConverter] Conversion failed: %s", relative_path);
        return false;
    }

    ConsolidationStats consolidationStats;
    ConsolidatePBRTextures(root_alias, consolidationStats, nullptr);

    return stats.textures_failed == 0;
}

} // namespace xray::render::pbr
