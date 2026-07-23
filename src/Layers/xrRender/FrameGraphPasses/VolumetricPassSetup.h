#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
class VolumetricRenderer;
}

namespace xray::render::fg::passes
{

struct ParticleBatch; // ParticlePassSetup.h

// Must match volumetric/shared_froxel.h FroxelParams (b5)
struct alignas(16) FroxelParamsCB
{
    u32 dims[3];
    float nearPlane;
    float farPlane;
    float invLogFarNear;
    float pad0[2];

    Fmatrix invVP;
    Fvector cameraPos;
    float pad1;

    float fogDensity;
    float fogHeightFalloff;
    float fogBaseHeight;
    float pad2;
    Fvector fogAlbedo;
    float pad3;
    Fvector sunDir;
    float pad4;
    Fvector sunColor;
    float sunIntensity;
    Fvector4 classicFogParams;
};
static_assert(sizeof(FroxelParamsCB) % 16 == 0, "FroxelParamsCB must be 16-byte aligned");

struct alignas(16) TemporalParamsCB
{
    Fmatrix prevVP;
    float temporalAlpha;
    float pad[3];
};
static_assert(sizeof(TemporalParamsCB) % 16 == 0);

struct alignas(16) ParticleInjectPointGPU
{
    Fvector position;
    float radius;
    Fvector albedo;
    float density;
};
static_assert(sizeof(ParticleInjectPointGPU) == 32);

struct alignas(16) ParticleInjectParamsCB
{
    u32 particleCount;
    u32 pad[3];
};

struct VolumetricPassState
{
    nvrhi::ComputePipelineHandle clearPipeline;
    nvrhi::ComputePipelineHandle fogPipeline;
    nvrhi::ComputePipelineHandle lightPipeline;
    nvrhi::ComputePipelineHandle particlePipeline;
    nvrhi::ComputePipelineHandle temporalPipeline;
    nvrhi::GraphicsPipelineHandle marchPipeline;
    nvrhi::BindingLayoutHandle clearLayout;
    nvrhi::BindingLayoutHandle fogLayout;
    nvrhi::BindingLayoutHandle lightLayout;
    nvrhi::BindingLayoutHandle particleLayout;
    nvrhi::BindingLayoutHandle temporalLayout;
    nvrhi::BindingLayoutHandle marchLayout;
    nvrhi::SamplerHandle linearSampler;
    nvrhi::BufferHandle paramsCB;
    nvrhi::BufferHandle temporalCB;
    nvrhi::BufferHandle particleParamsCB;
    nvrhi::BufferHandle particlePointsCB;
    Fmatrix prevVP;
    bool hasPrevVP = false;
    bool initialized = false;
    bool computeEnabled = false;
};

struct VolumetricPassData
{
    framegraph::VirtualResourceHandle sceneInput;
    framegraph::VirtualResourceHandle depthInput;
    framegraph::VirtualResourceHandle froxelVolume;
    framegraph::VirtualResourceHandle sceneOutput;
    VolumetricRenderer* volumetric = nullptr;
    VolumetricPassState* passState = nullptr;
    const xr_vector<ParticleBatch>* particleBatches = nullptr;
    u32 width = 0;
    u32 height = 0;
};

framegraph::VirtualResourceHandle setupVolumetricPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    VolumetricRenderer* volumetric,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle depth,
    u32 width,
    u32 height,
    VolumetricPassState& state,
    const xr_vector<ParticleBatch>* particleBatches = nullptr);

void InitializeVolumetricPass(nvrhi::IDevice* device, VolumetricPassState& state);
void ShutdownVolumetricPass(VolumetricPassState& state);

} // namespace xray::render::fg::passes
