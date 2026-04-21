#include "stdafx.h"
#include "ClusterLightPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::passes
{
using namespace framegraph;

struct ClusterLightPassData {
    fg::RenderDevice* device;
    ClusteredLightManager* lightManager;
    ClusterLightPassState* passState;
    u32 screenWidth;
    u32 screenHeight;
    VirtualResourceHandle hizPyramid;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    Fmatrix prevViewProj;
    bool useHiZ;
};

static void InitAssignPipeline(nvrhi::IDevice* nvDevice, ClusterLightPassState& state)
{
    if (state.assignInitialized)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto csResult = shaderLoader->LoadComputeShader("cluster_light_assign", "main");
    if (!csResult.handle) {
        Msg("! [ClusterLight] Failed to load assign compute shader");
        return;
    }
    state.assignShader = csResult.handle;

    auto* csRefl = shaderLoader->GetCachedReflection("cluster_light_assign", ".cs");
    if (!csRefl) {
        Msg("! [ClusterLight] Failed to get assign shader reflection");
        return;
    }

    state.assignLayout = GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(
        "ClusterLightAssign", *csRefl, nvDevice);
    if (!state.assignLayout)
        return;

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = state.assignShader;
    pipeDesc.bindingLayouts = { state.assignLayout };
    state.assignPipeline = nvDevice->createComputePipeline(pipeDesc);

    if (!state.assignPipeline)
        return;

    state.assignInitialized = true;
    Msg("* [ClusterLight] Assign pipeline initialized");
}

static void InitCullPipeline(nvrhi::IDevice* nvDevice, ClusterLightPassState& state)
{
    if (state.cullInitialized)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto csResult = shaderLoader->LoadComputeShader("light_hiz_cull", "main");
    if (!csResult.handle) {
        Msg("! [ClusterLight] Failed to load Hi-Z cull compute shader");
        return;
    }
    state.cullShader = csResult.handle;

    auto* csRefl = shaderLoader->GetCachedReflection("light_hiz_cull", ".cs");
    if (!csRefl) {
        Msg("! [ClusterLight] Failed to get cull shader reflection");
        return;
    }

    state.cullLayout = GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(
        "ClusterLightCull", *csRefl, nvDevice);
    if (!state.cullLayout)
        return;

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = state.cullShader;
    pipeDesc.bindingLayouts = { state.cullLayout };
    state.cullPipeline = nvDevice->createComputePipeline(pipeDesc);

    if (!state.cullPipeline)
        return;

    state.cullInitialized = true;
    Msg("* [ClusterLight] Hi-Z cull pipeline initialized");
}

void setupClusterLightPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    ClusteredLightManager* lightManager,
    u32 screenWidth,
    u32 screenHeight,
    ClusterLightPassState* state,
    VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const Fmatrix& prevViewProj,
    bool hasPrevViewProj)
{
    bool useHiZ = hasPrevViewProj && hizPyramid.is_valid() && hizWidth > 0 && hizHeight > 0;

    fg.addCallbackPass<ClusterLightPassData>(
        "ClusterLightAssign",
        [&, screenWidth, screenHeight, state, hizPyramid, hizWidth, hizHeight, hizMipLevels, prevViewProj, useHiZ](
            FrameGraph& builder, PassHandle passHandle, ClusterLightPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.sideEffects();

            if (useHiZ && hizPyramid.is_valid())
                passBuilder.read(hizPyramid);

            data.device = device;
            data.lightManager = lightManager;
            data.passState = state;
            data.screenWidth = screenWidth;
            data.screenHeight = screenHeight;
            data.hizPyramid = hizPyramid;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;
            data.prevViewProj = prevViewProj;
            data.useHiZ = useHiZ;
        },
        [](const ClusterLightPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("ClusterLightAssign", 18);

            if (!data.lightManager || data.lightManager->GetLightCount() == 0)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList || !nvDevice)
                return;

            InitAssignPipeline(nvDevice, *data.passState);
            if (!data.passState->assignInitialized)
                return;

            data.lightManager->Upload(cmdList);

            auto& cache = framegraph::GetPassResourceCache();
            bool didHiZCull = false;

            if (data.useHiZ)
            {
                InitCullPipeline(nvDevice, *data.passState);
                if (data.passState->cullInitialized)
                {
                    nvrhi::ITexture* hizTex = fg.GetPhysicalTexture(data.hizPyramid);
                    if (hizTex)
                    {
                        const u32 zero = 0;
                        cmdList->writeBuffer(data.lightManager->GetVisibleLightCountBuffer(), &zero, sizeof(u32));

                        LightHiZCullCB cullCB;
                        cullCB.prevViewProj = data.prevViewProj;
                        cullCB.curViewProj = Device.mFullTransform;
                        cullCB.cameraPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0);
                        cullCB.numLights = data.lightManager->GetLightCount();
                        cullCB.hizWidth = data.hizWidth;
                        cullCB.hizHeight = data.hizHeight;
                        cullCB.hizMipLevels = data.hizMipLevels;

                        auto cullParamsCB = cache.GetOrCreateVolatileCB(
                            "ClusterLightCull", "CullParams", sizeof(LightHiZCullCB), data.device, 16);
                        cmdList->writeBuffer(cullParamsCB, &cullCB, sizeof(cullCB));

                        auto* csRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("light_hiz_cull", ".cs");
                        if (csRefl)
                        {
                            framegraph::BindingSetBuilder bsb(*csRefl, nvDevice, "ClusterLightCull");
                            bsb.ConstantBuffer("LightHiZCullParams", cullParamsCB)
                               .BufferSRV("g_Lights", data.lightManager->GetLightDataBuffer())
                               .Texture("g_HiZPyramid", hizTex)
                               .BufferUAV("g_VisibleLightIndices", data.lightManager->GetVisibleLightIndicesBuffer())
                               .BufferUAV("g_VisibleLightCount", data.lightManager->GetVisibleLightCountBuffer());

                            auto cullBindingSet = cache.GetOrCreateBindingSet(
                                bsb.Build(), data.passState->cullLayout, nvDevice);

                            nvrhi::ComputeState cullState;
                            cullState.pipeline = data.passState->cullPipeline;
                            cullState.bindings = { cullBindingSet };
                            cmdList->setComputeState(cullState);

                            u32 groups = (data.lightManager->GetLightCount() + 63) / 64;
                            cmdList->dispatch(groups, 1, 1);

                            cmdList->commitBarriers();
                            if (psDeviceFlags.test(rsStatistic))
                                data.lightManager->ScheduleStatsReadback(cmdList);
                            didHiZCull = true;
                        }
                    }
                }
            }

            if (!didHiZCull)
                data.lightManager->UploadAllVisible(cmdList);

            float zNear = 0.2f;
            float zFar = g_pGamePersistent->Environment().CurrentEnv.far_plane;
            ClusterCB clusterCB = data.lightManager->BuildClusterCB(
                data.screenWidth, data.screenHeight, zNear, zFar);

            auto clusterParamsCB = cache.GetOrCreateVolatileCB(
                "ClusterLight", "ClusterParamsCB", sizeof(ClusterCB), data.device, 16);
            auto viewParamsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);

            cmdList->writeBuffer(clusterParamsCB, &clusterCB, sizeof(clusterCB));

            auto* csRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_light_assign", ".cs");
            if (!csRefl) return;

            framegraph::BindingSetBuilder bsb(*csRefl, nvDevice, "ClusterLightAssign");
            bsb.ConstantBuffer("ClusterParams", clusterParamsCB)
               .ConstantBuffer("static_globals", viewParamsCB)
               .BufferSRV("g_Lights", data.lightManager->GetLightDataBuffer())
               .BufferSRV("g_VisibleLightIndices", data.lightManager->GetVisibleLightIndicesBuffer())
               .BufferSRV("g_VisibleLightCount", data.lightManager->GetVisibleLightCountBuffer())
               .BufferUAV("g_ClusterGrid", data.lightManager->GetClusterGridBuffer())
               .BufferUAV("g_LightIndexList", data.lightManager->GetLightIndexListBuffer())
               .BufferUAV("g_LightIndexCounter", data.lightManager->GetLightIndexCounterBuffer());

            auto bindingSet = cache.GetOrCreateBindingSet(
                bsb.Build(), data.passState->assignLayout, nvDevice);

            nvrhi::ComputeState computeState;
            computeState.pipeline = data.passState->assignPipeline;
            computeState.bindings = { bindingSet };
            cmdList->setComputeState(computeState);

            u32 tilesX = data.lightManager->GetTilesX();
            u32 tilesY = data.lightManager->GetTilesY();
            cmdList->dispatch(tilesX, tilesY, CLUSTER_NUM_SLICES);
        }
    );
}

}
