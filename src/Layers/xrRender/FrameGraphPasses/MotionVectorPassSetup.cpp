#include "stdafx.h"
#include "MotionVectorPassSetup.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {
using namespace framegraph;

static void InitializeResources(ng::RenderDevice* device, MotionVectorPassState& state)
{
    if (state.initialized) return;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto csResult = RImplementation.m_shaderLoader->LoadComputeShader("restir_motion_vectors");
    if (!csResult.handle) return;

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("MotionVector", *csResult.reflection, nvDevice);

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = csResult.handle;
    pipeDesc.bindingLayouts = { state.layout };
    state.pipeline = cache.GetOrCreateComputePipeline("MotionVector", pipeDesc, nvDevice);

    state.cb = cache.GetOrCreateVolatileCB("MotionVector", "MotionVectorCB", 160, device);

    state.initialized = true;
}

MotionVectorOutput setupMotionVectorPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    VirtualResourceHandle depthInput,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    u32 width, u32 height,
    MotionVectorPassState& state)
{
    InitializeResources(device, state);

    if (!state.pipeline)
        return {};

    ResourceDesc mvDesc;
    mvDesc.type = ResourceDesc::Type::Texture2D;
    mvDesc.debugName = "rt_MotionVectors";
    mvDesc.width = width;
    mvDesc.height = height;
    mvDesc.format = nvrhi::Format::RG16_FLOAT;
    mvDesc.isUAV = true;
    mvDesc.isTransient = true;
    auto mvHandle = fg.CreateTexture("rt_MotionVectors", mvDesc);

    struct PassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle motionVectors;
        ng::RenderDevice* device;
        MotionVectorPassState* state;
        Fmatrix invViewProj;
        Fmatrix prevViewProj;
        u32 width, height;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "Motion Vectors",
        [&, mvHandle](FrameGraph& builder, PassHandle pass, PassData& data) {
            RenderPassBuilder pb(builder, pass);
            data.depth = pb.read(depthInput, ResourceState::ShaderResource);
            data.motionVectors = pb.write(mvHandle, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.invViewProj = invViewProj;
            data.prevViewProj = prevViewProj;
            data.width = width;
            data.height = height;
        },
        [](const PassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
            if (!depthTex || !mvTex) return;

            struct {
                Fmatrix invViewProj;
                Fmatrix prevViewProj;
                float screenW, screenH;
                float invScreenW, invScreenH;
            } cb;
            cb.invViewProj = data.invViewProj;
            cb.prevViewProj = data.prevViewProj;
            cb.screenW = (float)data.width;
            cb.screenH = (float)data.height;
            cb.invScreenW = 1.0f / data.width;
            cb.invScreenH = 1.0f / data.height;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &cb, sizeof(cb));

            auto* mvRefl = RImplementation.m_shaderLoader->GetCachedReflection("restir_motion_vectors", ".cs");
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
