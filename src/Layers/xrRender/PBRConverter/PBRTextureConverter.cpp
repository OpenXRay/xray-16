#include "stdafx.h"
#include "PBRTextureConverter.h"
#include "ONNXModelRunner.h"
#include "xrCore/FS.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include "Layers/xrRender/ResourceManager/DDSLoader.h"
#include "Layers/xrRender/ETextureParams.h"
#include <atomic>
#include <algorithm>
#include <DirectXTex.h>

using namespace xray::render::RENDER_NAMESPACE;
namespace xray::render::pbr {

extern ENGINE_API int ps_r4_use_pbr;

static void DecompressBC3AlphaBlock(const u8* blockData, u8* outAlpha) {
    u8 alpha0 = blockData[0];
    u8 alpha1 = blockData[1];

    u8 alphaPalette[8];
    alphaPalette[0] = alpha0;
    alphaPalette[1] = alpha1;

    if (alpha0 > alpha1) {
        alphaPalette[2] = (6 * alpha0 + 1 * alpha1) / 7;
        alphaPalette[3] = (5 * alpha0 + 2 * alpha1) / 7;
        alphaPalette[4] = (4 * alpha0 + 3 * alpha1) / 7;
        alphaPalette[5] = (3 * alpha0 + 4 * alpha1) / 7;
        alphaPalette[6] = (2 * alpha0 + 5 * alpha1) / 7;
        alphaPalette[7] = (1 * alpha0 + 6 * alpha1) / 7;
    } else {
        alphaPalette[2] = (4 * alpha0 + 1 * alpha1) / 5;
        alphaPalette[3] = (3 * alpha0 + 2 * alpha1) / 5;
        alphaPalette[4] = (2 * alpha0 + 3 * alpha1) / 5;
        alphaPalette[5] = (1 * alpha0 + 4 * alpha1) / 5;
        alphaPalette[6] = 0;
        alphaPalette[7] = 255;
    }

    u64 indices = 0;
    indices |= static_cast<u64>(blockData[2]);
    indices |= static_cast<u64>(blockData[3]) << 8;
    indices |= static_cast<u64>(blockData[4]) << 16;
    indices |= static_cast<u64>(blockData[5]) << 24;
    indices |= static_cast<u64>(blockData[6]) << 32;
    indices |= static_cast<u64>(blockData[7]) << 40;

    for (u32 i = 0; i < 16; ++i) {
        u32 index = static_cast<u32>((indices >> (i * 3)) & 0x7);
        outAlpha[i] = alphaPalette[index];
    }
}

static void DecompressBC1Block(const u8* blockData, u8* outPixels, bool hasAlpha) {
    u16 color0 = blockData[0] | (blockData[1] << 8);
    u16 color1 = blockData[2] | (blockData[3] << 8);

    u8 r0 = ((color0 >> 11) & 0x1F) * 255 / 31;
    u8 g0 = ((color0 >> 5) & 0x3F) * 255 / 63;
    u8 b0 = (color0 & 0x1F) * 255 / 31;

    u8 r1 = ((color1 >> 11) & 0x1F) * 255 / 31;
    u8 g1 = ((color1 >> 5) & 0x3F) * 255 / 63;
    u8 b1 = (color1 & 0x1F) * 255 / 31;

    u8 palette[4][4];
    palette[0][0] = r0; palette[0][1] = g0; palette[0][2] = b0; palette[0][3] = 255;
    palette[1][0] = r1; palette[1][1] = g1; palette[1][2] = b1; palette[1][3] = 255;

    if (color0 > color1 || !hasAlpha) {
        palette[2][0] = (2 * r0 + r1) / 3; palette[2][1] = (2 * g0 + g1) / 3; palette[2][2] = (2 * b0 + b1) / 3; palette[2][3] = 255;
        palette[3][0] = (r0 + 2 * r1) / 3; palette[3][1] = (g0 + 2 * g1) / 3; palette[3][2] = (b0 + 2 * b1) / 3; palette[3][3] = 255;
    } else {
        palette[2][0] = (r0 + r1) / 2; palette[2][1] = (g0 + g1) / 2; palette[2][2] = (b0 + b1) / 2; palette[2][3] = 255;
        palette[3][0] = 0; palette[3][1] = 0; palette[3][2] = 0; palette[3][3] = 0;
    }

    u32 indices = blockData[4] | (blockData[5] << 8) | (blockData[6] << 16) | (blockData[7] << 24);

    for (u32 i = 0; i < 16; ++i) {
        u32 index = (indices >> (i * 2)) & 0x3;
        outPixels[i * 4 + 0] = palette[index][0];
        outPixels[i * 4 + 1] = palette[index][1];
        outPixels[i * 4 + 2] = palette[index][2];
        outPixels[i * 4 + 3] = palette[index][3];
    }
}

static xr_vector<u8> DecompressDDS(const resources::DDSData& ddsData, u32& outWidth, u32& outHeight) {
    xr_vector<u8> result;

    if (!ddsData.isValid || ddsData.mipLevels.empty()) {
        return result;
    }

    const auto& mip0 = ddsData.mipLevels[0];
    outWidth = mip0.width;
    outHeight = mip0.height;

    const u32 pixelCount = outWidth * outHeight;
    result.resize(pixelCount * 4);

    bool isDXT1 = (static_cast<int>(ddsData.desc.format) == 56);
    bool isDXT5 = (static_cast<int>(ddsData.desc.format) == 60);
    bool isRGBA8 = (static_cast<int>(ddsData.desc.format) == 19);
    bool isR8 = (static_cast<int>(ddsData.desc.format) == 3);

    if (isRGBA8) {
        if (mip0.data && mip0.size >= pixelCount * 4) {
            std::memcpy(result.data(), mip0.data, pixelCount * 4);
        }
        return result;
    }

    if (isR8) {
        if (mip0.data && mip0.size >= pixelCount) {
            for (u32 i = 0; i < pixelCount; ++i) {
                u8 r = mip0.data[i];
                result[i * 4 + 0] = r;
                result[i * 4 + 1] = r;
                result[i * 4 + 2] = r;
                result[i * 4 + 3] = 255;
            }
        }
        return result;
    }

    if (!isDXT1 && !isDXT5) {
        Msg("! [PBRTextureConverter] Unsupported format: %d (only DXT1/DXT5/RGBA8/R8 supported)", static_cast<int>(ddsData.desc.format));
        return result;
    }

    const u32 blockSize = isDXT1 ? 8 : 16;
    const u32 blocksX = (outWidth + 3) / 4;
    const u32 blocksY = (outHeight + 3) / 4;

    const u8* srcData = mip0.data;

    for (u32 by = 0; by < blocksY; ++by) {
        for (u32 bx = 0; bx < blocksX; ++bx) {
            const u8* blockData = srcData + (by * blocksX + bx) * blockSize;
            u8 blockPixels[64];

            if (isDXT5) {
                u8 alphaValues[16];
                DecompressBC3AlphaBlock(blockData, alphaValues);
                DecompressBC1Block(blockData + 8, blockPixels, false);

                for (u32 i = 0; i < 16; ++i) {
                    blockPixels[i * 4 + 3] = alphaValues[i];
                }
            } else {
                DecompressBC1Block(blockData, blockPixels, true);
            }

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

static bool GenerateNormalMapFromDiffuse(
    const xr_vector<u8>& diffuseRGBA,
    u32 width, u32 height,
    xr_vector<u8>& outNormalRGBA,
    float amplitude = 12.0f)
{
    if (diffuseRGBA.empty() || width == 0 || height == 0) {
        return false;
    }

    DirectX::Image srcImage = {};
    srcImage.width = width;
    srcImage.height = height;
    srcImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcImage.rowPitch = width * 4;
    srcImage.slicePitch = width * height * 4;
    srcImage.pixels = const_cast<u8*>(diffuseRGBA.data());

    DirectX::ScratchImage normalMap;
    HRESULT hr = DirectX::ComputeNormalMap(
        srcImage,
        DirectX::CNMAP_CHANNEL_LUMINANCE,
        amplitude,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        normalMap
    );

    if (FAILED(hr)) {
        Msg("! [PBRTextureConverter] DirectXTex ComputeNormalMap failed: 0x%08X", hr);
        return false;
    }

    const DirectX::Image* result = normalMap.GetImage(0, 0, 0);
    if (!result || !result->pixels) {
        return false;
    }

    const u32 pixelCount = width * height;
    outNormalRGBA.resize(pixelCount * 4);
    const u8* src = result->pixels;

    if (pixelCount > 0) {
        Msg("* [PBRTextureConverter] DirectXTex normal[0]: R=%u G=%u B=%u A=%u",
            src[0], src[1], src[2], src[3]);
    }

    for (u32 i = 0; i < pixelCount; ++i) {
        u8 x = src[i * 4 + 0];
        u8 y = src[i * 4 + 1];
        u8 z = src[i * 4 + 2];

        outNormalRGBA[i * 4 + 0] = 128;
        outNormalRGBA[i * 4 + 1] = z;
        outNormalRGBA[i * 4 + 2] = y;
        outNormalRGBA[i * 4 + 3] = x;
    }

    if (pixelCount > 0) {
        Msg("* [PBRTextureConverter] X-Ray bump[0]: R=%u G=%u B=%u A=%u (Gloss,Z,Y,X)",
            outNormalRGBA[0], outNormalRGBA[1], outNormalRGBA[2], outNormalRGBA[3]);
    }

    return true;
}

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

    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;
    header.ddspf.dwGBitMask = 0x0000FF00;
    header.ddspf.dwBBitMask = 0x00FF0000;
    header.ddspf.dwABitMask = 0xFF000000;

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
static xr_unique_ptr<PBRPipeline> g_ai_pipeline;
static bool g_ai_available = false;

static Lock g_ai_pipeline_mutex;
#endif

static xr_string ExtractBaseName(const xr_string& path) {
    xr_string result = path;

    const auto dot = result.find_last_of('.');
    if (dot != xr_string::npos) {
        result = result.substr(0, dot);
    }

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

static bool FileExists(const char* root_alias, const char* relative_path) {
    string_path full_path;
    FS.update_path(full_path, root_alias, relative_path);
    return FS.exist(full_path);
}

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

static const char* const FOLDER_BLACKLIST[] = {
    "editor",
    "fx",
    "glow",
    "grad",
    "internal",
    "lights",
    "pfx",
    "sky",
    "ui",
    "intro",
    "terrain",
    "water",
    "wm",
};

static bool IsInBlacklistedFolder(const xr_string& path) {
    for (const char* folder : FOLDER_BLACKLIST) {
        xr_string prefix1 = xr_string(folder) + "\\";
        xr_string prefix2 = xr_string(folder) + "/";
        if (path.find(prefix1) == 0 || path.find(prefix2) == 0) {
            return true;
        }
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

    xr_map<xr_string, LegacyTextureAsset> asset_map;

    for (const auto& root : config.texture_roots) {
        FS_FileSet diffuse_files;

        const u32 flags = FS_ListFiles | (config.recursive ? 0 : FS_RootOnly);
        FS.file_list(diffuse_files, root.c_str(), flags, "*.dds");

        for (const auto& file : diffuse_files) {
            const xr_string file_name = file.name.c_str();

            if (!file_name.empty() && file_name[0] == '$') {
                continue;
            }

            if (IsInBlacklistedFolder(file_name)) {
                continue;
            }

            if (file_name.find("_bump") != xr_string::npos ||
                file_name.find("_bump#") != xr_string::npos ||
                file_name.find("_normal") != xr_string::npos ||
                file_name.find("_metallic") != xr_string::npos ||
                file_name.find("_roughness") != xr_string::npos ||
                file_name.find("_ao") != xr_string::npos ||
                file_name.find("_parallax") != xr_string::npos ||
                file_name.find("_pbr") != xr_string::npos ||
                file_name.find("_s.dds") != xr_string::npos ||
                file_name.find("_g.dds") != xr_string::npos) {
                continue;
            }

            const xr_string base_name = ExtractBaseName(file_name);
            if (base_name.empty()) {
                continue;
            }

            auto& asset = asset_map[base_name];
            asset.base_name = base_name;
            asset.diffuse = MakeFileLocation(root, file);
            asset.total_size_bytes = asset.diffuse.file_size;
            asset.latest_modified_time = asset.diffuse.modified_time_seconds;

            xr_string spec_path = base_name + "_s.dds";
            if (FileExists(root.c_str(), spec_path.c_str())) {
                asset.has_specular = true;
                asset.specular.root_alias = root;
                asset.specular.relative_path = spec_path;
            }

            xr_string gloss_path = base_name + "_g.dds";
            if (FileExists(root.c_str(), gloss_path.c_str())) {
                asset.has_gloss = true;
                asset.gloss.root_alias = root;
                asset.gloss.relative_path = gloss_path;
            }

            xr_string normal_path = base_name + "_bump.dds";
            if (FileExists(root.c_str(), normal_path.c_str())) {
                asset.has_normal = true;
                asset.normal.root_alias = root;
                asset.normal.relative_path = normal_path;
            } else {
                normal_path = base_name + "_bump#.dds";
                if (FileExists(root.c_str(), normal_path.c_str())) {
                    asset.has_normal = true;
                    asset.normal.root_alias = root;
                    asset.normal.relative_path = normal_path;
                }
            }
        }
    }

    inventory.assets.reserve(asset_map.size());
    for (auto& [base_name, asset] : asset_map) {
        inventory.assets.emplace_back(std::move(asset));

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

xr_string ComputeTextureInventoryDigest(const TextureInventory& inventory) {
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

    const u32 simple_hash = crc32(digest_input.c_str(), digest_input.size());

    char hex_buffer[16];
    xr_sprintf(hex_buffer, sizeof(hex_buffer), "%08x", simple_hash);
    return xr_string(hex_buffer);
}

static bool WriteTHMFileConsolidated(const char* root_alias, const xr_string& base_name, const xr_string& pbr_name);

static bool THMNeedsPBRUpdate(
    const char* root_alias,
    const xr_string& base_name,
    const xr_string& pbr_name)
{
    xr_string thm_path = base_name + ".thm";

    string_path full_path;
    FS.update_path(full_path, root_alias, thm_path.c_str());

    if (!FS.exist(full_path)) {
        return true;
    }

    IReader* reader = FS.r_open(root_alias, thm_path.c_str());
    if (!reader) {
        return true;
    }

    STextureParams params;
    params.Clear();
    params.Load(*reader);
    FS.r_close(reader);

    if (params.pbr_name != pbr_name.c_str()) {
        return true;
    }

    return false;
}

bool VerifyPBROutputs(
    const TextureInventory& inventory,
    const PBRConversionParams& params)
{
    u32 missing_textures = 0;
    u32 missing_thm = 0;

    for (const auto& asset : inventory.assets) {
        xr_string pbr_path = asset.base_name + "_pbr.dds";
        xr_string pbr_name = asset.base_name + "_pbr";

        if (FileExists(params.output_root.c_str(), pbr_path.c_str())) {
            if (THMNeedsPBRUpdate(params.output_root.c_str(), asset.base_name, pbr_name)) {
                missing_thm++;
            }
        } else {
            missing_textures++;
        }
    }

    if (missing_textures > 0) {
        Msg("[PBRTextureConverter] %u/%u textures need conversion",
            missing_textures, inventory.total_textures);
        return false;
    }

    if (missing_thm > 0) {
        Msg("[PBRTextureConverter] %u/%u .thm files need PBR fields updated",
            missing_thm, inventory.total_textures);
        return false;
    }

    return true;
}

struct ConvertedPBRTextures {
    xr_vector<u8> albedoData;
    xr_vector<u8> metallicData;
    xr_vector<u8> roughnessData;
    xr_vector<u8> aoData;
    xr_vector<u8> parallaxData;
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

    if (!diffuseData || !diffuseData->isValid || diffuseData->mipLevels.empty()) {
        return result;
    }

    const auto& baseMip = diffuseData->mipLevels[0];
    result.width = baseMip.width;
    result.height = baseMip.height;

    const u32 pixelCount = result.width * result.height;
    result.metallicData.resize(pixelCount, static_cast<u8>(params.default_metallic * 255.0f));
    result.roughnessData.resize(pixelCount, static_cast<u8>(params.default_roughness * 255.0f));

    if (specularData && specularData->isValid && !specularData->mipLevels.empty()) {
        const auto& specMip = specularData->mipLevels[0];

        if (specMip.width == result.width && specMip.height == result.height) {
            const u8* specPixels = specMip.data;

            for (u32 i = 0; i < pixelCount; ++i) {
                const u8 specR = specPixels[i * 4 + 0];
                result.metallicData[i] = specR;
            }
        }
    }

    if (glossData && glossData->isValid && !glossData->mipLevels.empty()) {
        const auto& glossMip = glossData->mipLevels[0];

        if (glossMip.width == result.width && glossMip.height == result.height) {
            const u8* glossPixels = glossMip.data;

            for (u32 i = 0; i < pixelCount; ++i) {
                const u8 glossValue = glossPixels[i * 4 + 0];
                result.roughnessData[i] = 255 - glossValue;
            }
        }
    }

    result.success = true;
    return result;
}

#ifdef USE_AI_PBR

static bool InitializeAIPipeline() {
    if (g_ai_pipeline) {
        return true;
    }

    PBRPipelineConfig config;
    config.use_gpu = true;
    config.verbose = true;

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

static ConvertedPBRTextures ConvertWithAI(
    const resources::DDSData* diffuseData,
    const resources::DDSData* normalData,
    const PBRConversionParams& params)
{
    ConvertedPBRTextures result;

    if (!diffuseData || !diffuseData->isValid || diffuseData->mipLevels.empty()) {
        return result;
    }
    if (!normalData || !normalData->isValid || normalData->mipLevels.empty()) {
        return result;
    }

    u32 diffuseWidth, diffuseHeight, normalWidth, normalHeight;
    xr_vector<u8> diffuseRGB = DecompressDDS(*diffuseData, diffuseWidth, diffuseHeight);
    xr_vector<u8> normalRGB = DecompressDDS(*normalData, normalWidth, normalHeight);

    if (diffuseRGB.empty() || normalRGB.empty()) {
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

    if (normalWidth != diffuseWidth || normalHeight != diffuseHeight) {
        Msg("~ [PBRTextureConverter] Upscaling normal map: %ux%u → %ux%u",
            normalWidth, normalHeight, diffuseWidth, diffuseHeight);

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

    xr_vector<u8> aiAlbedoRGB = outputs.albedo.ToImageData(false);
    result.metallicData = outputs.metallic.ToImageData(true);
    result.roughnessData = outputs.roughness.ToImageData(true);
    result.aoData = outputs.ao.ToImageData(true);
    result.parallaxData = outputs.parallax.ToImageData(true);

    const u32 pixelCount = result.width * result.height;
    result.albedoData.resize(pixelCount * 4);

    for (u32 i = 0; i < pixelCount; ++i) {
        result.albedoData[i * 4 + 0] = aiAlbedoRGB[i * 3 + 0];
        result.albedoData[i * 4 + 1] = aiAlbedoRGB[i * 3 + 1];
        result.albedoData[i * 4 + 2] = aiAlbedoRGB[i * 3 + 2];
        result.albedoData[i * 4 + 3] = diffuseRGB[i * 4 + 3];
    }

    result.success = true;
    return result;
}

#endif

static xr_vector<u8> GenerateMipLevel(const u8* srcData, u32 srcWidth, u32 srcHeight, u32 channels) {
    const u32 dstWidth = std::max(srcWidth / 2, 1u);
    const u32 dstHeight = std::max(srcHeight / 2, 1u);
    xr_vector<u8> dstData(dstWidth * dstHeight * channels);

    for (u32 y = 0; y < dstHeight; ++y) {
        for (u32 x = 0; x < dstWidth; ++x) {
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
    using namespace resources;

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
    header.dwPitchOrLinearSize = width * 4;
    header.dwDepth = 0;
    header.dwMipMapCount = mipCount;

    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;
    header.ddspf.dwGBitMask = 0x0000FF00;
    header.ddspf.dwBBitMask = 0x00FF0000;
    header.ddspf.dwABitMask = 0xFF000000;

    header.dwCaps = DDSCAPS_TEXTURE;
    if (generateMipmaps) {
        header.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }

    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));
    writer->w(&header, sizeof(header));

    const u32 pixelCount = width * height;
    writer->w(rgbaData, pixelCount * 4);

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

static bool WritePackedPBRDDS(
    const char* root_alias,
    const xr_string& relative_path,
    const u8* metallicData,
    const u8* roughnessData,
    const u8* aoData,
    const u8* parallaxData,
    u32 width,
    u32 height,
    bool generateMipmaps)
{
    const u32 pixelCount = width * height;
    xr_vector<u8> packedData(pixelCount * 4);

    for (u32 i = 0; i < pixelCount; ++i) {
        packedData[i * 4 + 0] = metallicData ? metallicData[i] : 0;
        packedData[i * 4 + 1] = roughnessData ? roughnessData[i] : 128;
        packedData[i * 4 + 2] = aoData ? aoData[i] : 255;
        packedData[i * 4 + 3] = parallaxData ? parallaxData[i] : 128;
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
    using namespace resources;

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
    header.dwPitchOrLinearSize = width;
    header.dwDepth = 0;
    header.dwMipMapCount = mipCount;

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

    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));

    writer->w(&header, sizeof(header));

    const u32 dataSize = width * height;
    writer->w(pixelData, dataSize);

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
    header.dwPitchOrLinearSize = width * 4;
    header.dwDepth = 0;
    header.dwMipMapCount = 1;

    header.ddspf.dwSize = 32;
    header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;
    header.ddspf.dwGBitMask = 0x0000FF00;
    header.ddspf.dwBBitMask = 0x00FF0000;
    header.ddspf.dwABitMask = 0xFF000000;

    header.dwCaps = DDSCAPS_TEXTURE;

    IWriter* writer = FS.w_open(root_alias, relative_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open file for writing: %s/%s", root_alias, relative_path.c_str());
        return false;
    }

    const u32 magic = DDS_MAGIC;
    writer->w(&magic, sizeof(magic));
    writer->w(&header, sizeof(header));
    writer->w(rgbaData, width * height * 4);

    FS.w_close(writer);
    Msg("* [PBRTextureConverter] Wrote uncompressed DDS: %s/%s (%ux%u)", root_alias, relative_path.c_str(), width, height);
    return true;
}

static void SanityTestDecompressor(const LegacyTextureAsset& asset)
{
    Msg("~ [PBRTextureConverter] SANITY TEST: Decompressing %s...", asset.diffuse.relative_path.c_str());

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

    u32 width, height;
    xr_vector<u8> rgba = DecompressDDS(diffuseData, width, height);

    if (rgba.empty()) {
        Msg("! [PBRTextureConverter] SANITY TEST: Decompression failed");
        return;
    }

    Msg("* [PBRTextureConverter] SANITY TEST: Decompressed to %ux%u RGBA8 (%u bytes)", width, height, rgba.size());

    WriteUncompressedDDS("$game_data$", "test_decompress_diffuse.dds", rgba.data(), width, height);

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

static bool WriteTHMFile(
    const char* root_alias,
    const xr_string& base_name,
    const xr_string& metallic_name,
    const xr_string& roughness_name,
    const xr_string& ao_name,
    const xr_string& parallax_name)
{
    xr_string thm_path = base_name + ".thm";

    STextureParams params;
    params.Clear();

    string_path full_path;
    FS.update_path(full_path, root_alias, thm_path.c_str());

    Msg("* [PBRTextureConverter] WriteTHMFile: Writing to %s (resolved: %s)", thm_path.c_str(), full_path);

    if (FS.exist(full_path)) {
        IReader* reader = FS.r_open(root_alias, thm_path.c_str());
        if (reader) {
            if (reader->find_chunk(THM_CHUNK_VERSION)) {
            }
            if (reader->find_chunk(THM_CHUNK_DATA)) {
            }
            params.Load(*reader);
            FS.r_close(reader);
            Msg("* [PBRTextureConverter] Loaded existing .thm, updating PBR fields");
        }
    } else {
        Msg("* [PBRTextureConverter] Creating new .thm file");
        params.type = STextureParams::ttImage;
    }

    params.metallic_name = metallic_name.c_str();
    params.roughness_name = roughness_name.c_str();
    params.ao_name = ao_name.c_str();
    params.parallax_name = parallax_name.c_str();

    IWriter* writer = FS.w_open(root_alias, thm_path.c_str());
    if (!writer) {
        Msg("! [PBRTextureConverter] Failed to open .thm for writing: %s/%s (full: %s)",
            root_alias, thm_path.c_str(), full_path);
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
    Msg("* [PBRTextureConverter] Successfully wrote .thm: %s", full_path);
    return true;
}

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
    std::atomic<u32> heuristic_conversions{0};
#endif

    const auto start_time = std::chrono::high_resolution_clock::now();

#ifdef USE_AI_PBR
    Msg("[PBRTextureConverter] Converting %u textures (AI: %s)...",
        total_textures, g_ai_available ? "enabled" : "disabled");
#else
    Msg("[PBRTextureConverter] Converting %u textures (heuristic mode)...", total_textures);
#endif

    for (size_t idx = 0; idx < inventory.assets.size(); ++idx) {
            LegacyTextureAsset asset = inventory.assets[idx];

            if (idx % 10 == 0 || idx == 0) {
                Msg("~ [PBRTextureConverter] Processing texture %u/%u: %s",
                    static_cast<u32>(idx + 1),
                    total_textures,
                    asset.base_name.c_str());
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

            xr_string albedo_path = asset.base_name + ".dds";
            xr_string metallic_path = asset.base_name + "_metallic.dds";
            xr_string roughness_path = asset.base_name + "_roughness.dds";
            xr_string ao_path = asset.base_name + "_ao.dds";
            xr_string parallax_path = asset.base_name + "_parallax.dds";

            xr_string metallic_name = asset.base_name + "_metallic";
            xr_string roughness_name = asset.base_name + "_roughness";
            xr_string ao_name = asset.base_name + "_ao";
            xr_string parallax_name = asset.base_name + "_parallax";

            Msg("* [PBRTextureConverter] PBR textures missing, running conversion: %s", asset.base_name.c_str());

            resources::DDSData diffuseData;
            resources::DDSData specularData;
            resources::DDSData glossData;

            auto StripDDSExtension = [](const xr_string& path) -> xr_string {
                if (path.size() >= 4 && path.substr(path.size() - 4) == ".dds") {
                    return path.substr(0, path.size() - 4);
                }
                return path;
            };

            xr_string diffuse_vfs_path = StripDDSExtension(asset.diffuse.relative_path);
            if (!resources::DDSLoader::LoadFromFile(diffuse_vfs_path.c_str(), diffuseData)) {
                Msg("! [PBRTextureConverter] Failed to load diffuse: %s (VFS path: %s)",
                    asset.diffuse.relative_path.c_str(), diffuse_vfs_path.c_str());
                failed_count++;
                continue;
            }

            xr_vector<u8> generatedNormalRGBA;

            if (!asset.has_normal) {
                Msg("~ [PBRTextureConverter] No normal map for %s, generating from diffuse...", asset.base_name.c_str());

                u32 diffuseWidth, diffuseHeight;
                xr_vector<u8> diffuseRGBA = DecompressDDS(diffuseData, diffuseWidth, diffuseHeight);

                if (!diffuseRGBA.empty()) {
                    if (GenerateNormalMapFromDiffuse(diffuseRGBA, diffuseWidth, diffuseHeight, generatedNormalRGBA, 4.0f)) {
                        xr_string bump_path = asset.base_name + "_bump.dds";
                        if (WriteNormalMapDDS(params.output_root.c_str(), bump_path, generatedNormalRGBA, diffuseWidth, diffuseHeight)) {
                            asset.has_normal = true;
                            asset.normal.root_alias = params.output_root;
                            asset.normal.relative_path = bump_path;
                            Msg("* [PBRTextureConverter] Generated normal map saved: %s", bump_path.c_str());
                        }
                    } else {
                        Msg("! [PBRTextureConverter] Failed to generate normal map for %s", asset.base_name.c_str());
                    }
                }
            }

#ifdef USE_AI_PBR
            if (!asset.has_normal) {
                Msg("~ [PBRTextureConverter] Skipping %s (no normal map available)", asset.base_name.c_str());
                skipped_count++;
                continue;
            }
#endif

            const resources::DDSData* specularPtr = nullptr;
            if (asset.has_specular) {
                xr_string spec_vfs_path = StripDDSExtension(asset.specular.relative_path);
                if (resources::DDSLoader::LoadFromFile(spec_vfs_path.c_str(), specularData)) {
                    specularPtr = &specularData;
                }
            }

            const resources::DDSData* glossPtr = nullptr;
            if (asset.has_gloss) {
                xr_string gloss_vfs_path = StripDDSExtension(asset.gloss.relative_path);
                if (resources::DDSLoader::LoadFromFile(gloss_vfs_path.c_str(), glossData)) {
                    glossPtr = &glossData;
                }
            }

            ConvertedPBRTextures converted;

#ifdef USE_AI_PBR
            if (g_ai_available && asset.has_normal) {
                resources::DDSData normalData;
                xr_string normal_vfs_path = StripDDSExtension(asset.normal.relative_path);

                if (resources::DDSLoader::LoadFromFile(normal_vfs_path.c_str(), normalData)) {
                    converted = ConvertWithAI(&diffuseData, &normalData, params);
                    if (converted.success) {
                        ai_conversions++;
                    }
                }
            }

            if (!converted.success) {
                converted = ConvertSpecularGlossToPBR(&diffuseData, specularPtr, glossPtr, params);
                if (converted.success) {
                    heuristic_conversions++;
                }
            }
#else
            converted = ConvertSpecularGlossToPBR(&diffuseData, specularPtr, glossPtr, params);
#endif

            if (!converted.success) {
                Msg("! [PBRTextureConverter] Conversion failed: %s", asset.base_name.c_str());
                failed_count++;
                continue;
            }

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

            if (!WriteSingleChannelDDS(params.output_root.c_str(), metallic_path,
                                       converted.metallicData.data(),
                                       converted.width, converted.height, params.generate_mipmaps)) {
                Msg("! [PBRTextureConverter] Failed to write metallic: %s/%s",
                    params.output_root.c_str(), metallic_path.c_str());
                failed_count++;
                continue;
            }

            if (!WriteSingleChannelDDS(params.output_root.c_str(), roughness_path,
                                       converted.roughnessData.data(),
                                       converted.width, converted.height, params.generate_mipmaps)) {
                Msg("! [PBRTextureConverter] Failed to write roughness: %s/%s",
                    params.output_root.c_str(), roughness_path.c_str());
                failed_count++;
                continue;
            }

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

            WriteTHMFile(params.output_root.c_str(), asset.base_name,
                        metallic_name, roughness_name, ao_name, parallax_name);

            converted_count++;

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

static xr_vector<u8> LoadSingleChannelDDS(const char* root_alias, const xr_string& path, u32& outWidth, u32& outHeight)
{
    xr_vector<u8> result;
    outWidth = outHeight = 0;

    xr_string vfs_path = path;
    if (vfs_path.size() >= 4 && vfs_path.substr(vfs_path.size() - 4) == ".dds") {
        vfs_path = vfs_path.substr(0, vfs_path.size() - 4);
    }

    resources::DDSData ddsData;
    if (!resources::DDSLoader::LoadFromFile(vfs_path.c_str(), ddsData)) {
        return result;
    }

    xr_vector<u8> rgba = DecompressDDS(ddsData, outWidth, outHeight);
    if (rgba.empty()) {
        return result;
    }

    const u32 pixelCount = outWidth * outHeight;
    result.resize(pixelCount);

    if (rgba.size() == pixelCount) {
        result = std::move(rgba);
    } else if (rgba.size() == pixelCount * 4) {
        for (u32 i = 0; i < pixelCount; ++i) {
            result[i] = rgba[i * 4];
        }
    }

    return result;
}

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

static bool DeleteFile(const char* root_alias, const xr_string& path)
{
    string_path full_path;
    FS.update_path(full_path, root_alias, path.c_str());

    if (!FS.exist(full_path)) {
        return true;
    }

    return (std::remove(full_path) == 0);
}

bool ConsolidatePBRTextures(
    const xr_string& root_alias,
    ConsolidationStats& out_stats,
    ProgressCallback progress_callback)
{
    Msg("[PBRTextureConverter] Starting PBR texture consolidation...");

    FS_FileSet metallic_files;
    FS.file_list(metallic_files, root_alias.c_str(), FS_ListFiles, "*_metallic.dds");

    out_stats.textures_found = static_cast<u32>(metallic_files.size());
    Msg("[PBRTextureConverter] Found %u texture sets to consolidate", out_stats.textures_found);

    if (metallic_files.empty()) {
        return true;
    }

    u32 processed = 0;
    for (const auto& file : metallic_files) {
        xr_string metallic_path = file.name.c_str();
        xr_string base_name = metallic_path.substr(0, metallic_path.size() - 13);

        xr_string roughness_path = base_name + "_roughness.dds";
        xr_string ao_path = base_name + "_ao.dds";
        xr_string parallax_path = base_name + "_parallax.dds";
        xr_string pbr_path = base_name + "_pbr.dds";

        string_path pbr_full_path;
        FS.update_path(pbr_full_path, root_alias.c_str(), pbr_path.c_str());
        if (FS.exist(pbr_full_path)) {
            processed++;
            continue;
        }

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

        xr_vector<u8> parallaxData = LoadSingleChannelDDS(root_alias.c_str(), parallax_path, tempW, tempH);
        if (!parallaxData.empty() && (tempW != width || tempH != height)) {
            parallaxData.clear();
        }

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

        xr_string pbr_name = base_name + "_pbr";
        WriteTHMFileConsolidated(root_alias.c_str(), base_name, pbr_name);

        if (DeleteFile(root_alias.c_str(), metallic_path)) out_stats.files_deleted++;
        if (DeleteFile(root_alias.c_str(), roughness_path)) out_stats.files_deleted++;
        if (DeleteFile(root_alias.c_str(), ao_path)) out_stats.files_deleted++;
        if (!parallaxData.empty() && DeleteFile(root_alias.c_str(), parallax_path)) out_stats.files_deleted++;

        out_stats.textures_consolidated++;
        processed++;

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

}
