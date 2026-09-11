#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"
#include "xrCore/Threading/ParallelFor.hpp"

namespace xray::render::pbr {

struct FileLocation {
    xr_string root_alias;
    xr_string relative_path;
    std::int64_t file_size = 0;
    std::int64_t modified_time_seconds = 0;
};

struct LegacyTextureAsset {
    xr_string base_name;

    FileLocation diffuse;
    FileLocation specular;
    FileLocation gloss;
    FileLocation normal;

    bool has_specular = false;
    bool has_gloss = false;
    bool has_normal = false;

    u32 diffuse_width = 0;
    u32 diffuse_height = 0;

    std::int64_t total_size_bytes = 0;
    std::int64_t latest_modified_time = 0;
};

struct PBRTextureOutputs {
    xr_string basecolor_path;
    xr_string metallic_path;
    xr_string roughness_path;
    xr_string ao_path;
    xr_string normal_path;

    bool needs_conversion = true;
};

struct TextureInventory {
    xr_vector<LegacyTextureAsset> assets;

    u32 total_textures = 0;
    u32 textures_with_specular = 0;
    u32 textures_with_gloss = 0;
    std::int64_t total_size_bytes = 0;
};

struct TextureScanConfig {
    xr_vector<xr_string> texture_roots = { "$game_textures$" };
    bool include_levels = true;
    bool recursive = true;
};

struct PBRConversionParams {
    xr_string output_root = "$game_textures$";

    bool generate_mipmaps = true;
    bool compress_output = true;

    float default_metallic = 0.0f;
    float default_roughness = 0.5f;
    float default_ao = 1.0f;

    u32 max_parallel_conversions = 0;
};

struct PBRConversionStats {
    u32 textures_scanned = 0;
    u32 textures_converted = 0;
    u32 textures_skipped = 0;
    u32 textures_failed = 0;

    std::int64_t bytes_read = 0;
    std::int64_t bytes_written = 0;

    float conversion_time_seconds = 0.0f;
};

using ProgressCallback = std::function<void(
    float progress,
    const char* status_text
)>;

TextureInventory BuildTextureInventory(const TextureScanConfig& config);

xr_string ComputeTextureInventoryDigest(const TextureInventory& inventory);

bool VerifyPBROutputs(
    const TextureInventory& inventory,
    const PBRConversionParams& params
);

bool ConvertTexturesToPBR(
    const TextureInventory& inventory,
    const PBRConversionParams& params,
    PBRConversionStats& out_stats,
    ProgressCallback progress_callback = nullptr
);

void ShutdownAIPipeline();

struct ConsolidationStats {
    u32 textures_found = 0;
    u32 textures_consolidated = 0;
    u32 textures_failed = 0;
    u32 files_deleted = 0;
};

bool ConvertSingleTextureToPBR(
    const char* root_alias,
    const char* relative_path,
    const PBRConversionParams& params
);

bool ConsolidatePBRTextures(
    const xr_string& root_alias,
    ConsolidationStats& out_stats,
    ProgressCallback progress_callback = nullptr
);

inline TextureInventory BuildDefaultTextureInventory() {
    TextureScanConfig config;
    config.texture_roots = { "$game_textures$" };
    config.include_levels = true;
    config.recursive = true;
    return BuildTextureInventory(config);
}

}
