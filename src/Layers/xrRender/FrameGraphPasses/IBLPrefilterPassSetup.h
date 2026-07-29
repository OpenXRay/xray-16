#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
namespace fg {
class RenderDevice;
}
}

namespace xray::render::framegraph {
class FrameGraph;
}

namespace xray::render::fg::passes {

constexpr u32 kIBLSpecSize = 128;
constexpr u32 kIBLBrdfSize = 128;
constexpr u32 kIBLSHCount = 9;
constexpr u32 kIBLMaxProbes = 16;
constexpr u32 kIBLProbeStride = 16;
constexpr u32 kIBLProbeSize = 64;
constexpr u32 kIBLSlotEnvBRDF = 52;
constexpr u32 kIBLSlotSkySH = 53;
constexpr u32 kIBLSlotProbes = 54;
constexpr u32 kIBLSlotProbeSpec = 55;

struct IBLProbeCPU
{
    Fvector position;
    float radius = 8.f;
    Fvector boxMin;
    Fvector boxMax;
    float intensity = 1.f;
    float blend = 0.f;
    bool dirty = true;
    bool ready = false;
};

struct IBLPrefilterPassState
{
    nvrhi::ComputePipelineHandle prefilterPipe;
    nvrhi::BindingLayoutHandle prefilterLayout;
    nvrhi::ComputePipelineHandle shPipe;
    nvrhi::BindingLayoutHandle shLayout;
    nvrhi::ComputePipelineHandle brdfPipe;
    nvrhi::BindingLayoutHandle brdfLayout;
    nvrhi::ComputePipelineHandle probeCapturePipe;
    nvrhi::BindingLayoutHandle probeCaptureLayout;

    nvrhi::TextureHandle spec0;
    nvrhi::TextureHandle spec1;
    nvrhi::TextureHandle brdfLut;
    nvrhi::TextureHandle probeCubeArray;
    nvrhi::TextureHandle probeCaptureTemp;
    nvrhi::BufferHandle shBuffer;
    nvrhi::BufferHandle probeBuffer;
    nvrhi::BufferHandle prefilterCB;
    nvrhi::BufferHandle shCB;
    nvrhi::BufferHandle brdfCB;
    nvrhi::BufferHandle probeCaptureCB;

    nvrhi::ITexture* lastSrc0 = nullptr;
    nvrhi::ITexture* lastSrc1 = nullptr;
    bool brdfReady = false;
    bool resourcesReady = false;
    bool pipelinesReady = false;
    bool needsPrefilter = true;
    bool hasBakedOnce = false;
    float prefilterBlend = 0.f;
    u32 prefilterStep = 0;
    u32 mipCount = 1;
    u32 probeCount = 0;
    IBLProbeCPU probes[kIBLMaxProbes];
    int pendingCapture = -1;
    u32 captureStep = 0;
    nvrhi::ITexture* lastAutoSky0 = nullptr;
    nvrhi::ITexture* lastAutoSky1 = nullptr;
};

struct IBLBindResources
{
    nvrhi::ITexture* spec0 = nullptr;
    nvrhi::ITexture* spec1 = nullptr;
    nvrhi::ITexture* brdfLut = nullptr;
    nvrhi::IBuffer* shBuffer = nullptr;
    nvrhi::IBuffer* probeBuffer = nullptr;
    nvrhi::ITexture* probeCubeArray = nullptr;
    bool prefilterEnabled = false;
    u32 mipCount = 1;
};

void InitializeIBLPrefilter(fg::RenderDevice* device, IBLPrefilterPassState& state);

IBLBindResources setupIBLPrefilterPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    nvrhi::ITexture* srcSky0,
    nvrhi::ITexture* srcSky1,
    IBLPrefilterPassState& state);

void setupIBLProbeCapturePass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    nvrhi::ITexture* srcSky0,
    nvrhi::ITexture* srcSky1,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle sceneDepth,
    IBLPrefilterPassState& state);

void BindIBLResources(framegraph::BindingSetBuilder& bsb, const IBLBindResources& ibl, nvrhi::IDevice* nv);
void SetCurrentIBLBindResources(const IBLBindResources& ibl);
const IBLBindResources& GetCurrentIBLBindResources();
IBLPrefilterPassState* GetActiveIBLState();

int IBLAddProbeAtCamera(IBLPrefilterPassState& state, float radius = 8.f);
void IBLClearProbes(IBLPrefilterPassState& state);
void IBLRequestCapture(IBLPrefilterPassState& state, int probeIndex);
void IBLUpdateAutoProbes(IBLPrefilterPassState& state, nvrhi::ITexture* srcSky0, nvrhi::ITexture* srcSky1);
void IBLUploadProbeBuffer(IBLPrefilterPassState& state, nvrhi::ICommandList* cmd, nvrhi::IDevice* nv);
void IBLUpdateFrameMeta(IBLPrefilterPassState& state, nvrhi::ICommandList* cmd, float ssgiSkyScale);

} // namespace xray::render::fg::passes
