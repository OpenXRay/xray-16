#pragma once

#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "xrCore/xrstring.h"

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  VOLATILE CONSTANT BUFFER POOL
// ═══════════════════════════════════════════════════
//
// Manages a pool of volatile constant buffers with automatic deduplication.
// Multiple shaders with the same CB layout share the same VCB instance.
//
// Key benefits:
// - Memory efficiency: Only creates VCBs for layouts that are actually used
// - Ring buffer efficiency: Each VCB is sized exactly for its data, avoiding waste
// - Automatic deduplication: Same layout = same VCB instance
//
class VolatileConstantBufferPool {
public:
    VolatileConstantBufferPool(ng::RenderDevice* device);
    ~VolatileConstantBufferPool();

    // ═══════════════════════════════════════════════════
    //  VCB LAYOUT IDENTIFICATION
    // ═══════════════════════════════════════════════════

    // Unique identifier for a constant buffer layout
    struct CBLayout {
        shared_str name;      // e.g. "dynamic_transforms", "Material", "PerObjectWithBones"
        u32 slot;             // Register slot (b0, b1, ...)
        u32 size;             // Size in bytes

        CBLayout() : slot(0), size(0) {}
        CBLayout(const char* n, u32 s, u32 sz) : name(n), slot(s), size(sz) {}

        // Comparison for map lookup
        bool operator<(const CBLayout& other) const {
            if (slot != other.slot) return slot < other.slot;
            if (size != other.size) return size < other.size;
            return xr_strcmp(name.c_str(), other.name.c_str()) < 0;
        }

        // Equality check
        bool operator==(const CBLayout& other) const {
            return slot == other.slot &&
                   size == other.size &&
                   xr_strcmp(name.c_str(), other.name.c_str()) == 0;
        }
    };

    // ═══════════════════════════════════════════════════
    //  VCB MANAGEMENT
    // ═══════════════════════════════════════════════════

    // Get or create a VCB for the given layout
    // Returns existing VCB if layout was already registered
    ng::BufferHandle GetOrCreateVCB(const CBLayout& layout);

    // Get VCB by layout (returns invalid handle if not found)
    ng::BufferHandle GetVCB(const CBLayout& layout) const;

    // Check if a layout is already registered
    bool HasLayout(const CBLayout& layout) const;

    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct Stats {
        u32 numLayouts = 0;        // Number of unique CB layouts
        u32 numVCBs = 0;           // Number of VCB instances (should equal numLayouts)
        u32 totalMemory = 0;       // Total memory allocated for all VCBs
    };

    Stats GetStats() const;
    void LogStats() const;

private:
    ng::RenderDevice* m_device;

    // Map from layout to VCB
    xr_map<CBLayout, ng::BufferHandle> m_vcbs;

    // Statistics
    mutable Stats m_stats;
};

} // namespace xray::render::framegraph
