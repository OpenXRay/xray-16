// xrRender/Bindless/BindlessTypes.h
// 2D Grid-Packing Texture Atlas Types
// 4 texture atlases (Diffuse, Normal, Detail, PBR), each using 4096x4096 pages
// with 128px cell-based grid packing for variable-size textures
#pragma once

#include "xrCore/xrCore.h"
#include <bitset>

namespace xray::render::RENDER_NAMESPACE::bindless {

// ═══════════════════════════════════════════════════════
//  CONFIGURATION
// ═══════════════════════════════════════════════════════

// Atlas page size - 4096x4096 fits all game textures (64MB per page RGBA8)
constexpr u32 ATLAS_PAGE_SIZE = 4096;

// Cell size for grid allocation - 128px aligns with power-of-2 textures
constexpr u32 ATLAS_CELL_SIZE = 128;

// Grid dimensions: 4096/128 = 32 cells per axis
constexpr u32 ATLAS_GRID_SIZE = ATLAS_PAGE_SIZE / ATLAS_CELL_SIZE;  // 32
constexpr u32 ATLAS_CELLS_PER_PAGE = ATLAS_GRID_SIZE * ATLAS_GRID_SIZE;  // 1024

// Maximum pages per atlas (64 pages × 64MB = 4GB max per atlas type)
constexpr u32 MAX_PAGES_PER_ATLAS = 64;

// Maximum unique materials
constexpr u32 MAX_MATERIALS = 16384;

// ═══════════════════════════════════════════════════════
//  ATLAS TYPES
// ═══════════════════════════════════════════════════════

enum class AtlasType : u8 {
    Diffuse  = 0,   // Base color / albedo
    Normal   = 1,   // Normal maps
    Detail   = 2,   // Detail textures
    PBR      = 3,   // Metallic/Roughness/AO packed
    Count    = 4
};

constexpr u32 TOTAL_ATLAS_COUNT = static_cast<u32>(AtlasType::Count);  // 4

inline const char* GetAtlasTypeName(AtlasType type) {
    static const char* names[] = { "Diffuse", "Normal", "Detail", "PBR" };
    return names[static_cast<u8>(type)];
}

// ═══════════════════════════════════════════════════════
//  ATLAS ALLOCATION (8 bytes - packs to 2× u32)
// ═══════════════════════════════════════════════════════
// Stores UV rect information for a texture within an atlas page
//
// Packing layout:
//   Low u32:  pageIndex (16 bits) | cellX (16 bits)
//   High u32: cellY (16 bits) | cellsWide (8 bits) | cellsTall (8 bits)

struct AtlasAllocation {
    u16 pageIndex;   // Which page (0-65535)
    u16 cellX;       // Start cell X (0-31)
    u16 cellY;       // Start cell Y (0-31)
    u8  cellsWide;   // Width in cells (1-32)
    u8  cellsTall;   // Height in cells (1-32)

    AtlasAllocation() : pageIndex(0xFFFF), cellX(0), cellY(0), cellsWide(0), cellsTall(0) {}

    AtlasAllocation(u16 page, u16 x, u16 y, u8 w, u8 h)
        : pageIndex(page), cellX(x), cellY(y), cellsWide(w), cellsTall(h) {}

    bool IsValid() const { return pageIndex != 0xFFFF && cellsWide > 0 && cellsTall > 0; }

    // Pack to 2× u32 for GPU transfer
    u32 GetPackedLow() const {
        return static_cast<u32>(pageIndex) | (static_cast<u32>(cellX) << 16);
    }

    u32 GetPackedHigh() const {
        return static_cast<u32>(cellY) |
               (static_cast<u32>(cellsWide) << 16) |
               (static_cast<u32>(cellsTall) << 24);
    }

    // Unpack from 2× u32
    static AtlasAllocation Unpack(u32 low, u32 high) {
        AtlasAllocation alloc;
        alloc.pageIndex = static_cast<u16>(low & 0xFFFF);
        alloc.cellX = static_cast<u16>((low >> 16) & 0xFFFF);
        alloc.cellY = static_cast<u16>(high & 0xFFFF);
        alloc.cellsWide = static_cast<u8>((high >> 16) & 0xFF);
        alloc.cellsTall = static_cast<u8>((high >> 24) & 0xFF);
        return alloc;
    }

    // Get pixel coordinates
    u32 GetPixelX() const { return cellX * ATLAS_CELL_SIZE; }
    u32 GetPixelY() const { return cellY * ATLAS_CELL_SIZE; }
    u32 GetPixelWidth() const { return cellsWide * ATLAS_CELL_SIZE; }
    u32 GetPixelHeight() const { return cellsTall * ATLAS_CELL_SIZE; }
};
static_assert(sizeof(AtlasAllocation) == 8, "AtlasAllocation must be 8 bytes");

// ═══════════════════════════════════════════════════════
//  ATLAS PAGE (occupancy tracking)
// ═══════════════════════════════════════════════════════
// Tracks which cells are occupied in a 32×32 grid

struct AtlasPage {
    std::bitset<ATLAS_CELLS_PER_PAGE> occupied;  // 1024 bits = 128 bytes
    u32 freeCount = ATLAS_CELLS_PER_PAGE;

    AtlasPage() = default;

    // Check if a rectangular region can be placed at (x, y)
    bool CanPlace(u32 x, u32 y, u32 w, u32 h) const {
        if (x + w > ATLAS_GRID_SIZE || y + h > ATLAS_GRID_SIZE)
            return false;

        for (u32 cy = y; cy < y + h; ++cy) {
            for (u32 cx = x; cx < x + w; ++cx) {
                if (occupied[cy * ATLAS_GRID_SIZE + cx])
                    return false;
            }
        }
        return true;
    }

    // Mark a rectangular region as occupied
    void MarkOccupied(u32 x, u32 y, u32 w, u32 h) {
        for (u32 cy = y; cy < y + h; ++cy) {
            for (u32 cx = x; cx < x + w; ++cx) {
                u32 idx = cy * ATLAS_GRID_SIZE + cx;
                if (!occupied[idx]) {
                    occupied[idx] = true;
                    --freeCount;
                }
            }
        }
    }

    // Check if page has enough free cells (quick reject)
    bool HasSpace(u32 cellsNeeded) const {
        return freeCount >= cellsNeeded;
    }
};

// ═══════════════════════════════════════════════════════
//  MATERIAL DATA (matches HLSL MaterialData struct)
// ═══════════════════════════════════════════════════════
// GPU-side material representation - must match HLSL exactly!
// Uses packed atlas allocations for texture UV rects
//
// Layout (48 bytes total):
//   Bytes 0-7:   Diffuse atlas (low + high)
//   Bytes 8-15:  Normal atlas (low + high)
//   Bytes 16-23: Detail atlas (low + high)
//   Bytes 24-31: PBR atlas (low + high)
//   Bytes 32-47: Material properties

struct alignas(16) MaterialData {
    // Packed atlas allocations (2× u32 each)
    u32 diffuseAtlasLow;
    u32 diffuseAtlasHigh;
    u32 normalAtlasLow;
    u32 normalAtlasHigh;

    u32 detailAtlasLow;
    u32 detailAtlasHigh;
    u32 pbrAtlasLow;
    u32 pbrAtlasHigh;

    // Material properties
    float detailScale;
    float alphaRef;
    u32 flags;
    float padding1;
};
static_assert(sizeof(MaterialData) == 48, "MaterialData must be 48 bytes for GPU alignment");

// Helper to set atlas allocation in MaterialData
inline void SetDiffuseAllocation(MaterialData& mat, const AtlasAllocation& alloc) {
    mat.diffuseAtlasLow = alloc.GetPackedLow();
    mat.diffuseAtlasHigh = alloc.GetPackedHigh();
}

inline void SetNormalAllocation(MaterialData& mat, const AtlasAllocation& alloc) {
    mat.normalAtlasLow = alloc.GetPackedLow();
    mat.normalAtlasHigh = alloc.GetPackedHigh();
}

inline void SetDetailAllocation(MaterialData& mat, const AtlasAllocation& alloc) {
    mat.detailAtlasLow = alloc.GetPackedLow();
    mat.detailAtlasHigh = alloc.GetPackedHigh();
}

inline void SetPBRAllocation(MaterialData& mat, const AtlasAllocation& alloc) {
    mat.pbrAtlasLow = alloc.GetPackedLow();
    mat.pbrAtlasHigh = alloc.GetPackedHigh();
}

// Material flags (must match HLSL)
enum MaterialFlags : u32 {
    MAT_FLAG_ALPHA_TEST    = (1 << 0),
    MAT_FLAG_TWO_SIDED     = (1 << 1),
    MAT_FLAG_EMISSIVE      = (1 << 2),
    MAT_FLAG_HAS_DETAIL    = (1 << 3),
    MAT_FLAG_HAS_NORMAL    = (1 << 4),
    MAT_FLAG_HAS_PBR       = (1 << 5),
};

// ═══════════════════════════════════════════════════════
//  LEGACY SUPPORT (for gradual migration)
// ═══════════════════════════════════════════════════════
// These will be removed after full migration to grid packing

// Old slice-based reference (deprecated)
struct TextureSliceRef {
    u32 packed = 0xFFFFFFFF;

    TextureSliceRef() = default;

    TextureSliceRef(AtlasType type, u32 sliceIndex) {
        packed = (static_cast<u32>(type) << 24) | (sliceIndex & 0x00FFFFFF);
    }

    bool IsValid() const { return packed != 0xFFFFFFFF; }
    AtlasType GetType() const { return static_cast<AtlasType>(packed >> 24); }
    u32 GetSliceIndex() const { return packed & 0x00FFFFFF; }
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
