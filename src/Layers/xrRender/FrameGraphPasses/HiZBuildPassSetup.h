// xrRender/FrameGraphPasses/HiZBuildPassSetup.h
#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

// Forward declarations
namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// ═══════════════════════════════════════════════════════
//  HI-Z PYRAMID BUILD PASS (Phase 3.5: GPU Culling Foundation)
// ═══════════════════════════════════════════════════════
//
// PURPOSE:
// Generates a hierarchical depth (Hi-Z) pyramid from the depth buffer.
// Each mip level contains the MAX depth of a 2x2 block from the previous level.
// This creates a conservative depth pyramid for GPU occlusion culling.
//
// PIPELINE:
// 1. Mip 0: Read 2x2 from full-res depth → write to Hi-Z mip 0
// 2. Mip N: Read 2x2 from Hi-Z mip N-1 → write to Hi-Z mip N
// 3. Repeat until 1x1 mip
//
// OUTPUT:
// - R32_FLOAT texture with full mip chain
// - Mip 0 = half resolution, Mip N = 1x1
//
// USAGE:
// - GPU occlusion culling (object_cull.cs)
// - Froxel volumetrics ray march termination (Phase 6)
// - Particle depth fade
//
// PERFORMANCE:
// - ~0.2-0.3ms for 1080p (async compute)
// - log2(max(width,height)) dispatches
//
// REFERENCES:
// - Wihlidal: "Optimizing the Graphics Pipeline with Compute" (GDC 2016)
// - Ubisoft: "GPU-Driven Rendering Pipelines" (SIGGRAPH 2015)

struct HiZBuildPassState {
    nvrhi::SamplerHandle pointSampler;
    nvrhi::ComputePipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    bool initialized = false;
    bool computeEnabled = false;
    u32 currentWidth = 0;
    u32 currentHeight = 0;
    u32 mipLevels = 0;
};

struct HiZBuildData {
    framegraph::VirtualResourceHandle depthInput;
    framegraph::VirtualResourceHandle hizPyramid;
    ng::RenderDevice* device;
    u32 width;
    u32 height;
    u32 mipLevels;
    HiZBuildPassState* passState;
};

struct HiZPyramidOutput {
    framegraph::VirtualResourceHandle pyramid;  // R32_FLOAT with full mip chain
    u32 mipLevels;                              // Number of mip levels generated
    u32 width;                                  // Base width (same as depth buffer)
    u32 height;                                 // Base height
};

void InitializeHiZResources(ng::RenderDevice* device, u32 width, u32 height, HiZBuildPassState& state);

// Setup Hi-Z pyramid generation pass
// Input: Full-resolution depth buffer from depth prepass
// Output: Depth pyramid with conservative (max) depths
//
// NOTE: This pass runs on the compute queue (async compute capable)
// and can overlap with other graphics work.
HiZPyramidOutput setupHiZBuildPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    u32 width,
    u32 height,
    HiZBuildPassState& state
);

// Get number of mip levels for given dimensions
inline u32 CalculateHiZMipLevels(u32 width, u32 height) {
    u32 mipLevels = 1;
    u32 w = width, h = height;
    while (w > 1 || h > 1) {
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        mipLevels++;
    }
    return mipLevels;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
