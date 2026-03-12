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
    ng::RenderDevice* device;
    ClusteredLightManager* lightManager;
    ClusterLightPassState* passState;
    u32 screenWidth;
    u32 screenHeight;
};

static void InitClusterLightPipeline(nvrhi::IDevice* nvDevice, ClusterLightPassState& state)
{
    if (state.initialized)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto csResult = shaderLoader->LoadComputeShader("cluster_light_assign", "main");
    if (!csResult.handle) {
        Msg("! [ClusterLight] Failed to load compute shader");
        return;
    }
    state.computeShader = csResult.handle;

    auto* csRefl = shaderLoader->GetCachedReflection("cluster_light_assign", ".cs");
    if (!csRefl) {
        Msg("! [ClusterLight] Failed to get compute shader reflection");
        return;
    }

    state.bindingLayout = GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(
        "ClusterLight", *csRefl, nvDevice);
    if (!state.bindingLayout) {
        Msg("! [ClusterLight] Failed to create binding layout from reflection");
        return;
    }

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = state.computeShader;
    pipeDesc.bindingLayouts = { state.bindingLayout };
    state.pipeline = nvDevice->createComputePipeline(pipeDesc);

    if (!state.pipeline) {
        Msg("! [ClusterLight] Failed to create compute pipeline");
        return;
    }

    state.initialized = true;
    Msg("* [ClusterLight] Compute pipeline initialized");
}

void setupClusterLightPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    ClusteredLightManager* lightManager,
    u32 screenWidth,
    u32 screenHeight,
    ClusterLightPassState* state)
{
    fg.addCallbackPass<ClusterLightPassData>(
        "ClusterLightAssign",
        [&, screenWidth, screenHeight, state](
            FrameGraph& builder, PassHandle passHandle, ClusterLightPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.sideEffects();

            data.device = device;
            data.lightManager = lightManager;
            data.passState = state;
            data.screenWidth = screenWidth;
            data.screenHeight = screenHeight;
        },
        [](const ClusterLightPassData& data, const FrameGraph& fg, ng::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("ClusterLightAssign", 18);

            if (!data.lightManager || data.lightManager->GetLightCount() == 0)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList || !nvDevice)
                return;

            InitClusterLightPipeline(nvDevice, *data.passState);
            if (!data.passState->initialized)
                return;

            data.lightManager->Upload(cmdList);

            float zNear = 0.2f;
            float zFar = g_pGamePersistent->Environment().CurrentEnv.far_plane;
            ClusterCB clusterCB = data.lightManager->BuildClusterCB(
                data.screenWidth, data.screenHeight, zNear, zFar);

            StaticGlobals staticGlobals = BuildStaticGlobals();

            auto& cache = framegraph::GetPassResourceCache();
            auto clusterParamsCB = cache.GetOrCreateVolatileCB(
                "ClusterLight", "ClusterParamsCB", sizeof(ClusterCB), data.device, 16);
            auto viewParamsCB = cache.GetOrCreateVolatileCB(
                "ClusterLight", "ViewParamsCB", sizeof(StaticGlobals), data.device, 16);

            cmdList->writeBuffer(clusterParamsCB, &clusterCB, sizeof(clusterCB));
            cmdList->writeBuffer(viewParamsCB, &staticGlobals, sizeof(staticGlobals));

            auto* csRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cluster_light_assign", ".cs");
            if (!csRefl) return;

            framegraph::BindingSetBuilder bsb(*csRefl, nvDevice, "ClusterLight");
            bsb.ConstantBuffer("ClusterParams", clusterParamsCB)
               .ConstantBuffer("static_globals", viewParamsCB)
               .BufferSRV("g_Lights", data.lightManager->GetLightDataBuffer())
               .BufferUAV("g_ClusterGrid", data.lightManager->GetClusterGridBuffer())
               .BufferUAV("g_LightIndexList", data.lightManager->GetLightIndexListBuffer())
               .BufferUAV("g_LightIndexCounter", data.lightManager->GetLightIndexCounterBuffer());

            auto bindingSet = cache.GetOrCreateBindingSet(
                bsb.Build(), data.passState->bindingLayout, nvDevice);

            nvrhi::ComputeState computeState;
            computeState.pipeline = data.passState->pipeline;
            computeState.bindings = { bindingSet };
            cmdList->setComputeState(computeState);

            u32 tilesX = data.lightManager->GetTilesX();
            u32 tilesY = data.lightManager->GetTilesY();
            cmdList->dispatch(tilesX, tilesY, CLUSTER_NUM_SLICES);
        }
    );
}

}
