// xrRender/FrameGraph/IPass.h
#pragma once

#include "FrameGraph.h"
#include "ShaderReflection.h"

namespace xray::render {
    struct GeometryBatch;  // Forward declaration

    namespace fg {
        class RenderContext;  // Forward declaration
    }
}

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  OUTPUT LAYOUTS (Render Target Structures)
// ═══════════════════════════════════════════════════
// Shared render target layouts used by multiple passes.
// Future: Add LightingOutputLayout, PostProcessOutputLayout, etc.

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;      // RT0: Lit HDR color (RGBA16_FLOAT)
    VirtualResourceHandle normal;      // RT1: World normal.xyz + Roughness.a (RGBA16_FLOAT)
    VirtualResourceHandle baseColor;   // RT2: Unlit diffuse albedo.rgb + Metallic.a (RGBA8_UNORM)
    VirtualResourceHandle worldPos;    // RT3: World position.xyz (RGBA32_FLOAT)
    VirtualResourceHandle depth;       // Depth/Stencil (D32)
    VirtualResourceHandle distortion;  // Distortion buffer (RG = UV offset, A = intensity)
};

// ═══════════════════════════════════════════════════
//  PASS INTERFACE (All passes inherit from this)
// ═══════════════════════════════════════════════════
//
// Base interface for all render passes in the dynamic routing system.
// Passes are created dynamically based on material requirements and
// automatically receive batches that match their rendering phase.
//
// Usage:
//   class MyPass : public IPass {
//       void Setup(FrameGraph& fg) override {
//           // Declare resources
//       }
//       void Execute(fg::RenderContext& ctx, const FrameGraph& fg) override {
//           // Draw batches
//       }
//   };
//
class IPass {
public:
    virtual ~IPass() = default;

    // ═══════════════════════════════════════════════════
    //  CORE INTERFACE
    // ═══════════════════════════════════════════════════

    // Setup pass in FrameGraph (declare resources, add pass)
    // Called once per frame during BuildFrameGraph()
    virtual void Setup(FrameGraph& fg) = 0;

    // Execute pass (called by FrameGraph during Execute())
    // Draw all assigned batches using the provided context
    virtual void Execute(fg::RenderContext& ctx, const FrameGraph& fg) = 0;

    // Get phase this pass handles
    virtual RenderPhase GetPhase() const = 0;

    // ═══════════════════════════════════════════════════
    //  BATCH MANAGEMENT
    // ═══════════════════════════════════════════════════

    // Assign batches to this pass (called by routing system)
    virtual void SetBatches(const xr_vector<GeometryBatch*>& batches) {
        m_batches = batches;
    }

    // Get assigned batches (for debugging/stats)
    const xr_vector<GeometryBatch*>& GetBatches() const {
        return m_batches;
    }

    // Get batch count
    u32 GetBatchCount() const {
        return static_cast<u32>(m_batches.size());
    }

protected:
    // ═══════════════════════════════════════════════════
    //  PASS REGISTRATION HELPERS
    // ═══════════════════════════════════════════════════

    // Resource I/O declaration for pass registration
    struct PassIO {
        xr_vector<std::pair<VirtualResourceHandle, ResourceState>> reads;
        xr_vector<std::pair<VirtualResourceHandle, ResourceState>> writes;
    };

    // Auto-register pass with framegraph
    // This handles AddPass(), PassRead/Write(), and SetPassCallback()
    void RegisterPass(FrameGraph& fg, const char* name, const PassIO& io) {
        // Create pass
        PassHandle pass = fg.AddPass(name);

        // Declare reads
        for (const auto& [resource, state] : io.reads) {
            fg.PassRead(pass, resource, state);
        }

        // Declare writes
        for (const auto& [resource, state] : io.writes) {
            fg.PassWrite(pass, resource, state);
        }

        // Set execution callback (calls derived class's Execute())
        fg.SetPassCallback(pass, [this](fg::RenderContext& ctx, const FrameGraph& fg) {
            this->Execute(ctx, fg);
        });
    }

public:
    // ═══════════════════════════════════════════════════
    //  STATISTICS
    // ═══════════════════════════════════════════════════

    struct PassStats {
        u32 numDrawCalls = 0;
        u32 numTriangles = 0;
        u32 numBatches = 0;
        float cpuTimeMs = 0.0f;
        float gpuTimeMs = 0.0f;
    };

    virtual const PassStats& GetStats() const {
        return m_stats;
    }

public:
    // Helper: Get phase name for logging (public for use by FrameGraphRenderer)
    static const char* GetPhaseName(RenderPhase phase) {
        switch (phase) {
            case RenderPhase::Geometry:    return "Geometry";
            case RenderPhase::Lighting:    return "Lighting";
            case RenderPhase::Combine:     return "Combine";
            case RenderPhase::PostProcess: return "PostProcess";
            case RenderPhase::Shadow:      return "Shadow";
            case RenderPhase::Custom:      return "Custom";
            default:                       return "Unknown";
        }
    }

protected:
    // Assigned batches (populated by routing system)
    xr_vector<GeometryBatch*> m_batches;

    // Statistics (updated by Execute())
    PassStats m_stats;
};

} // namespace xray::render::framegraph
