// xrRender/PBR/PBRTextureConverter.h
// Phase 2.5: PBR Texture Conversion System
//
// PURPOSE:
// Automatically converts legacy X-Ray textures (diffuse/specular/gloss)
// to PBR workflow (basecolor/metallic/roughness) at startup when r4_use_pbr=1
//
// DESIGN:
// - Scans $game_textures$ for diffuse textures (_d.dds suffix)
// - For each diffuse, looks for matching specular (_s.dds) and gloss (_g.dds)
// - Converts in parallel using xr_parallel_for (like OGF→OZZX conversion)
// - Generates NEW textures alongside originals:
//     wood_d.dds       → wood_d.dds (basecolor, converted in-place)
//     wood_s.dds       → wood_metallic.dds (NEW)
//     wood_s.dds+gloss → wood_roughness.dds (NEW)
// - Digest-based caching: Only converts if source textures changed
//
// NAMING CONVENTION:
// - Keeps vanilla names intact (wood_d.dds stays wood_d.dds)
// - Adds new PBR-specific textures with descriptive suffixes
// - Shaders detect which mode via r4_use_pbr flag

#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"
#include "xrCore/Threading/ParallelFor.hpp"

namespace xray::render::pbr {

// ══════════════════════════════════════════════════════════
//  FILE LOCATION (From OGF conversion pattern)
// ══════════════════════════════════════════════════════════

struct FileLocation {
    xr_string root_alias;        // e.g., "$game_textures$"
    xr_string relative_path;     // e.g., "wood\wood_d.dds"
    std::int64_t file_size = 0;
    std::int64_t modified_time_seconds = 0;
};

// ══════════════════════════════════════════════════════════
//  LEGACY TEXTURE ASSET (Input)
// ══════════════════════════════════════════════════════════
// Represents a set of legacy textures that need PBR conversion:
// - Diffuse (_d.dds)      → Required
// - Specular (_s.dds)     → Optional (fallback: derive from diffuse)
// - Gloss (_g.dds)        → Optional (fallback: constant roughness)
// - Normal (_bump.dds)    → Passthrough (no conversion needed)

struct LegacyTextureAsset {
    xr_string base_name;         // "wood\wood" (without suffix)

    FileLocation diffuse;        // wood_d.dds (required)
    FileLocation specular;       // wood_s.dds (optional)
    FileLocation gloss;          // wood_g.dds (optional)
    FileLocation normal;         // wood_bump.dds (optional, passthrough)

    bool has_specular = false;
    bool has_gloss = false;
    bool has_normal = false;

    u32 diffuse_width = 0;
    u32 diffuse_height = 0;

    // For digest calculation (detect changes)
    std::int64_t total_size_bytes = 0;
    std::int64_t latest_modified_time = 0;
};

// ══════════════════════════════════════════════════════════
//  PBR TEXTURE OUTPUTS (Generated)
// ══════════════════════════════════════════════════════════

struct PBRTextureOutputs {
    xr_string basecolor_path;     // wood_d.dds (converted in-place)
    xr_string metallic_path;      // wood_metallic.dds (NEW)
    xr_string roughness_path;     // wood_roughness.dds (NEW)
    xr_string ao_path;            // wood_ao.dds (NEW, optional)
    xr_string normal_path;        // wood_bump.dds (passthrough)

    bool needs_conversion = true; // False if outputs already exist and are up-to-date
};

// ══════════════════════════════════════════════════════════
//  TEXTURE INVENTORY (All assets to convert)
// ══════════════════════════════════════════════════════════

struct TextureInventory {
    xr_vector<LegacyTextureAsset> assets;

    u32 total_textures = 0;
    u32 textures_with_specular = 0;
    u32 textures_with_gloss = 0;
    std::int64_t total_size_bytes = 0;
};

// ══════════════════════════════════════════════════════════
//  SCAN CONFIGURATION
// ══════════════════════════════════════════════════════════

struct TextureScanConfig {
    xr_vector<xr_string> texture_roots = { "$game_textures$" };
    bool include_levels = true;  // Also scan $level$ for per-level textures
    bool recursive = true;
};

// ══════════════════════════════════════════════════════════
//  CONVERSION PARAMETERS
// ══════════════════════════════════════════════════════════

struct PBRConversionParams {
    xr_string output_root = "$game_textures$";  // Where to write converted textures

    // Conversion quality settings
    bool generate_mipmaps = true;
    bool compress_output = true;  // Use DXT compression

    // Fallback values when source textures missing
    float default_metallic = 0.0f;    // Non-metallic by default
    float default_roughness = 0.5f;   // Mid-range roughness
    float default_ao = 1.0f;          // No occlusion by default

    // Performance
    u32 max_parallel_conversions = 0; // 0 = use all cores
};

// ══════════════════════════════════════════════════════════
//  CONVERSION STATISTICS
// ══════════════════════════════════════════════════════════

struct PBRConversionStats {
    u32 textures_scanned = 0;
    u32 textures_converted = 0;
    u32 textures_skipped = 0;   // Already up-to-date
    u32 textures_failed = 0;

    std::int64_t bytes_read = 0;
    std::int64_t bytes_written = 0;

    float conversion_time_seconds = 0.0f;
};

// ══════════════════════════════════════════════════════════
//  PROGRESS CALLBACK
// ══════════════════════════════════════════════════════════

using ProgressCallback = std::function<void(
    float progress,          // 0.0 to 1.0
    const char* status_text  // "Converting wood_d.dds..."
)>;

// ══════════════════════════════════════════════════════════
//  PUBLIC API
// ══════════════════════════════════════════════════════════

// Scan filesystem for legacy textures needing conversion
TextureInventory BuildTextureInventory(const TextureScanConfig& config);

// Compute digest (hash) of inventory for change detection
// Returns hex string like "a3f5c9..." (similar to OGF digest)
xr_string ComputeTextureInventoryDigest(const TextureInventory& inventory);

// Check if PBR outputs exist and are up-to-date
bool VerifyPBROutputs(
    const TextureInventory& inventory,
    const PBRConversionParams& params
);

// Convert all textures in inventory (parallel processing)
bool ConvertTexturesToPBR(
    const TextureInventory& inventory,
    const PBRConversionParams& params,
    PBRConversionStats& out_stats,
    ProgressCallback progress_callback = nullptr
);

// ══════════════════════════════════════════════════════════
//  PBR TEXTURE CONSOLIDATION
// ══════════════════════════════════════════════════════════
// Packs existing separate PBR textures (_metallic, _roughness, _ao, _parallax)
// into a single _pbr.dds texture (R=M, G=R, B=AO, A=Parallax)

struct ConsolidationStats {
    u32 textures_found = 0;
    u32 textures_consolidated = 0;
    u32 textures_failed = 0;
    u32 files_deleted = 0;
};

// Convert a single texture to PBR at the given VFS location
// Produces <base_name>_pbr.dds (packed R=M, G=R, B=AO, A=Parallax) alongside the original
// Returns true if conversion succeeded or output already exists
bool ConvertSingleTextureToPBR(
    const char* root_alias,
    const char* relative_path,
    const PBRConversionParams& params
);

// Consolidate existing separate PBR textures into packed _pbr.dds
// Also updates .thm files and deletes old separate files
bool ConsolidatePBRTextures(
    const xr_string& root_alias,
    ConsolidationStats& out_stats,
    ProgressCallback progress_callback = nullptr
);

// ══════════════════════════════════════════════════════════
//  HELPER: Build default inventory (common case)
// ══════════════════════════════════════════════════════════

inline TextureInventory BuildDefaultTextureInventory() {
    TextureScanConfig config;
    config.texture_roots = { "$game_textures$" };
    config.include_levels = true;
    config.recursive = true;
    return BuildTextureInventory(config);
}

} // namespace xray::render::pbr
