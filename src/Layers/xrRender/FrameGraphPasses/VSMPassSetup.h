#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::fg::passes {

constexpr u32 kVSMLevels = 6;
constexpr u32 kVSMVirtualRes = 4096;
constexpr u32 kVSMPageSize = 128;
constexpr u32 kVSMPagesAxis = kVSMVirtualRes / kVSMPageSize;
constexpr u32 kVSMPagesPerLevel = kVSMPagesAxis * kVSMPagesAxis;
constexpr u32 kVSMPageCount = kVSMLevels * kVSMPagesPerLevel;
constexpr float kVSMZNear = -1000.0f;
constexpr float kVSMZFar = 1000.0f;
constexpr float kVSMZSnap = 256.0f;

struct VsmParams {
    Fmatrix view;
    Fvector4 level[kVSMLevels];
    Fvector4 zparams;
};
static_assert(sizeof(VsmParams) == 176, "VsmParams layout is shader-visible");

struct VSMState {
    static constexpr u32 kReadbackSlots = 6;

    bool active = false;
    VsmParams params;
    Fmatrix sunView;
    s32 pageBase[kVSMLevels][2] = {};
    s32 pageBasePrev[kVSMLevels][2] = {};
    bool pageBasePrevValid = false;
    Fvector prevSunDir;
    bool prevSunValid = false;
    bool sunMoving = false;
    float zCentre = 0.0f;
    bool zCentreValid = false;
    u32 frame = 0;
    u32 invalidations = 0;
    u32 snapMax = 0;
    float sunStepMax = 0.0f;

    nvrhi::BufferHandle needed;
    nvrhi::BufferHandle counter;
    nvrhi::BufferHandle readback[kReadbackSlots];
    u32 readbackWrite = 0;
    u32 readbackScheduled = 0;

    u32 markPages = 0;
    u32 levelPages[kVSMLevels] = {};

    nvrhi::ComputePipelineHandle markPipeline;
    nvrhi::BindingLayoutHandle markLayout;
    nvrhi::ComputePipelineHandle debugPipeline;
    nvrhi::BindingLayoutHandle debugLayout;
    bool pipelinesFailed = false;
    u32 lastLogTime = 0;
};

struct VSMMarkOutput {
    framegraph::VirtualResourceHandle needed;
    framegraph::VirtualResourceHandle debugView;
    bool active = false;
};

void InvalidateVSMCache(VSMState& state);
void VSMBeginFrame(VSMState& state, const Fvector& camPos, const Fvector& sunDir);

VSMMarkOutput setupVSMMarkPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depth,
    framegraph::VirtualResourceHandle orderAfter,
    u32 width,
    u32 height,
    VSMState* state,
    xray::profiler::GPUProfiler* gpuProfiler);

}
