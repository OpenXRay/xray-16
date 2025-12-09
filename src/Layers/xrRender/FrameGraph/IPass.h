#pragma once

#include "FrameGraph.h"
#include "ShaderReflection.h"

namespace xray::render {
    struct GeometryBatch;

    namespace ng {
        class RenderContext;
    }
}

namespace xray::render::framegraph {

struct DefaultOutputLayout {
    VirtualResourceHandle albedo;
    VirtualResourceHandle normal;
    VirtualResourceHandle material;
    VirtualResourceHandle depth;
    VirtualResourceHandle distortion;
};

class IPass {
public:
    virtual ~IPass() = default;

    virtual void Setup(FrameGraph& fg) = 0;

    virtual void Execute(ng::RenderContext& ctx, const FrameGraph& fg) = 0;

    virtual RenderPhase GetPhase() const = 0;

    virtual void SetBatches(const xr_vector<GeometryBatch*>& batches) {
        m_batches = batches;
    }

    const xr_vector<GeometryBatch*>& GetBatches() const {
        return m_batches;
    }

    u32 GetBatchCount() const {
        return static_cast<u32>(m_batches.size());
    }

protected:

    struct PassIO {
        xr_vector<std::pair<VirtualResourceHandle, ResourceState>> reads;
        xr_vector<std::pair<VirtualResourceHandle, ResourceState>> writes;
    };

    void RegisterPass(FrameGraph& fg, const char* name, const PassIO& io) {
        PassHandle pass = fg.AddPass(name);

        for (const auto& [resource, state] : io.reads) {
            fg.PassRead(pass, resource, state);
        }

        for (const auto& [resource, state] : io.writes) {
            fg.PassWrite(pass, resource, state);
        }

        fg.SetPassCallback(pass, [this](ng::RenderContext& ctx, const FrameGraph& fg) {
            this->Execute(ctx, fg);
        });
    }

public:

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
    xr_vector<GeometryBatch*> m_batches;

    PassStats m_stats;
};

}
