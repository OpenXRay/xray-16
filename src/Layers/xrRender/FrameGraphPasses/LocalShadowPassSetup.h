#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraphPasses/ForwardColorPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/ShadowPassSetup.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg
{
class RenderDevice;
class GPUCullingManager;
}

namespace xray::render::fg::passes
{

struct alignas(16) LocalSkinInstanceGPU
{
    Fmatrix world;
    u32 materialID = 0;
    u32 boneOffset = 0;
    u32 pad0 = 0;
    u32 pad1 = 0;
};
static_assert(sizeof(LocalSkinInstanceGPU) == 80);

struct LocalSkinPipeVariant
{
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::ShaderHandle vs;
    nvrhi::BindingSetHandle bindingSet;
};

struct LocalShadowPassState
{
    xray::render::fg::TextureHandle atlasHandle;
    nvrhi::ITexture* atlas = nullptr;
    nvrhi::TextureHandle staticCache;
    nvrhi::ITexture* staticCacheTex = nullptr;
    nvrhi::TextureHandle esmAtlas;
    nvrhi::ITexture* esmAtlasTex = nullptr;
    nvrhi::TextureHandle esmTemp;
    nvrhi::ITexture* esmTempTex = nullptr;
    nvrhi::BufferHandle cascadeCB;
    nvrhi::BufferHandle esmCB;
    u32 cascadeCBMaxVersions = 0;
    nvrhi::GraphicsPipelineHandle opaquePipeline;
    nvrhi::GraphicsPipelineHandle foliagePipeline;
    nvrhi::ComputePipelineHandle esmConvertPipeline;
    nvrhi::ComputePipelineHandle esmBlurPipeline;
    nvrhi::BindingLayoutHandle esmConvertLayout;
    nvrhi::BindingLayoutHandle esmBlurLayout;
    LocalSkinPipeVariant skin1w;
    LocalSkinPipeVariant skinHq;
    LocalSkinPipeVariant skin2w;
    LocalSkinPipeVariant skin3w;
    LocalSkinPipeVariant skin4w;
    nvrhi::BufferHandle localSkinInstanceSSBO;
    u32 localSkinInstanceCapacity = 0;
    nvrhi::BufferHandle localSkinIndirectArgs;
    u32 localSkinIndirectArgsCapacity = 0;
    nvrhi::BufferHandle cullDoneMarker;
    nvrhi::FramebufferHandle pageFramebuffers[MAX_LOCAL_SHADOW_PAGES];
    nvrhi::GraphicsPipelineHandle clearDepthPipeline;
    u32 resolution = 0;
    u32 sliceCount = 0;
    bool initialized = false;
    bool enabled = false;
    bool allocFailed = false;
    nvrhi::TextureHandle atlasOwned;
};

struct LocalShadowOutputs
{
    framegraph::VirtualResourceHandle atlas;
    nvrhi::ITexture* atlasTex = nullptr;
    nvrhi::ITexture* esmTex = nullptr;
    bool valid = false;
};

void InitializeLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state);
void ShutdownLocalShadowPass(fg::RenderDevice* device, LocalShadowPassState& state);

LocalShadowOutputs setupLocalShadowPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const BindlessForwardConfig& bindlessConfig,
    ShadowPassState& shadowState,
    LocalShadowPassState& state,
    const xr_vector<xray::render::GeometryBatch>* worldSkinnedBatches = nullptr,
    fg::GPUCullingManager* gpuCulling = nullptr,
    framegraph::VirtualResourceHandle gpuCullDrawArgs = {});

} // namespace xray::render::fg::passes
