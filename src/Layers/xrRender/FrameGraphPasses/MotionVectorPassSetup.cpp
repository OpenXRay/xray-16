#include "stdafx.h"
#include "MotionVectorPassSetup.h"
#include "TAAPassSetup.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {
using namespace framegraph;

namespace
{
constexpr u32 kMotionVectorPipeVersion = 9;
}

static void InitializeResources(fg::RenderDevice* device, MotionVectorPassState& state)
{
    if (state.initialized && state.pipeVersion == kMotionVectorPipeVersion && state.pipeline)
        return;

    state.initialized = false;
    state.pipeVersion = 0;
    state.pipeline = nullptr;
    state.layout = nullptr;
    state.cb = nullptr;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    BindingSetBuilder::InvalidateReflectionCache();
    auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_motion_vectors");
    if (!csResult.handle || !csResult.reflection)
        return;

    state.layout = cache.GetOrCreateBindingLayoutFromReflection(
        "MotionVector_v3", *csResult.reflection, nvDevice);

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = csResult.handle;
    pipeDesc.bindingLayouts = { state.layout };
    state.pipeline = cache.GetOrCreateComputePipeline("MotionVector_v3", pipeDesc, nvDevice);

    state.cb = cache.GetOrCreateVolatileCB("MotionVector", "MotionVectorCB_v5", 320, device);

    state.initialized = true;
    state.pipeVersion = kMotionVectorPipeVersion;
}

MotionVectorOutput setupMotionVectorPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle depthInput,
    const Fmatrix& viewProj,
    const Fmatrix& prevViewProj,
    const Fmatrix& invViewProjJittered,
    u32 width, u32 height,
    MotionVectorPassState& state)
{
    InitializeResources(device, state);

    if (!state.pipeline || !depthInput.is_valid())
        return {};

    ResourceDesc mvDesc;
    mvDesc.type = ResourceDesc::Type::Texture2D;
    mvDesc.debugName = "rt_MotionVectors";
    mvDesc.width = width;
    mvDesc.height = height;
    mvDesc.format = nvrhi::Format::RGBA16_FLOAT;
    mvDesc.isUAV = true;
    mvDesc.isTransient = false;
    auto mvHandle = fg.CreateTexture("rt_MotionVectors", mvDesc);

    const Fvector cameraPos = Device.vCameraPosition;
    const bool hasPrevCamera = state.hasPrevCamera;
    const Fvector prevCameraPos = state.prevCameraPos;
    state.prevCameraPos = cameraPos;
    state.hasPrevCamera = true;

    struct PassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle motionVectors;
        fg::RenderDevice* device;
        MotionVectorPassState* state;
        Fmatrix viewProj;
        Fmatrix prevViewProj;
        Fmatrix invViewProj;
        Fvector cameraPos;
        Fvector prevCameraPos;
        u32 width, height;
        u32 hasPrevCamera;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "Motion Vectors",
        [&, mvHandle](FrameGraph& builder, PassHandle pass, PassData& data) {
            RenderPassBuilder pb(builder, pass);
            data.depth = pb.read(depthInput, ResourceState::ShaderResource);
            data.motionVectors = pb.write(mvHandle, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.viewProj = viewProj;
            data.prevViewProj = prevViewProj;
            data.invViewProj = invViewProjJittered;
            data.cameraPos = cameraPos;
            data.prevCameraPos = prevCameraPos;
            data.width = width;
            data.height = height;
            data.hasPrevCamera = hasPrevCamera ? 1u : 0u;
        },
        [](const PassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto* depthTex = fgGraph.GetPhysicalTexture(data.depth);
            auto* mvTex = fgGraph.GetPhysicalTexture(data.motionVectors);
            if (!depthTex || !mvTex) return;

            struct alignas(16) {
                Fmatrix viewProj;
                Fmatrix prevViewProj;
                Fmatrix invViewProj;
                float screenW, screenH;
                float invScreenW, invScreenH;
                Fvector4 cameraPos;
                Fvector4 prevCameraPos;
                u32 hasPrevCamera;
                float currJitterX, currJitterY;
                float prevJitterX, prevJitterY;
                float pad0, pad1, pad2;
            } cb;
            cb.viewProj = data.viewProj;
            cb.prevViewProj = data.prevViewProj;
            cb.invViewProj = data.invViewProj;
            cb.screenW = (float)data.width;
            cb.screenH = (float)data.height;
            cb.invScreenW = 1.0f / data.width;
            cb.invScreenH = 1.0f / data.height;
            cb.cameraPos.set(data.cameraPos.x, data.cameraPos.y, data.cameraPos.z, 0.f);
            cb.prevCameraPos.set(data.prevCameraPos.x, data.prevCameraPos.y, data.prevCameraPos.z, 0.f);
            cb.hasPrevCamera = data.hasPrevCamera;
            cb.currJitterX = g_taa_jitter_px;
            cb.currJitterY = g_taa_jitter_py;
            cb.prevJitterX = g_taa_jitter_prev_px;
            cb.prevJitterY = g_taa_jitter_prev_py;
            cb.pad0 = cb.pad1 = cb.pad2 = 0;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &cb, sizeof(cb));

            auto* mvRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_motion_vectors", ".cs");
            BindingSetBuilder bsb(*mvRefl, nvDevice, "MotionVector");
            bsb.ConstantBuffer("MotionVectorParams", data.state->cb)
               .Texture("t_Depth", depthTex)
               .TextureUAV("u_MotionVectors", mvTex);
            auto bindDesc = bsb.Build();
            auto& cache = GetPassResourceCache();
            auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, data.state->layout, nvDevice);

            ctx->SetComputePipeline(data.state->pipeline.Get());
            ctx->SetComputeBindingSet(0, bindingSet.Get());
            ctx->Dispatch((data.width + 7) / 8, (data.height + 7) / 8, 1);
        }
    );

    return { passData.motionVectors };
}

} // namespace
