// xrRender/FrameGraph/IPass.h
#pragma once

#include "FrameGraph.h"
#include "ShaderReflection.h"

namespace xray::render {
    struct GeometryBatch;  // Forward declaration

    namespace ng {
        class RenderContext;  // Forward declaration
    }
}

namespace xray::render::framegraph {

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
//       GBufferOutputs Setup(FrameGraph& fg) override {
//           // Declare resources
//       }
//       void Execute(ng::RenderContext& ctx, const FrameGraph& fg) override {
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
    virtual void Execute(ng::RenderContext& ctx, const FrameGraph& fg) = 0;

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
