#include "stdafx.h"
#include "MotionVectorPassSetup.h"
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

static void InitializeResources(fg::RenderDevice* device, MotionVectorPassState& state)
{
    if (state.initialized) return;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("restir_motion_vectors");
    if (!csResult.handle) return;

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("MotionVector", *csResult.reflection, nvDevice);

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = csResult.handle;
    pipeDesc.bindingLayouts = { state.layout };
    state.pipeline = cache.GetOrCreateComputePipeline("MotionVector", pipeDesc, nvDevice);

    state.cb = cache.GetOrCreateVolatileCB("MotionVector", "MotionVectorCB", 160, device);

    nvrhi::TextureDesc dummyDesc;
    dummyDesc.width = 1;
    dummyDesc.height = 1;
    dummyDesc.format = nvrhi::Format::R32_UINT;
    dummyDesc.debugName = "MotionVector_DummyVisID";
    dummyDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    dummyDesc.keepInitialState = true;
    state.dummyVisId = nvDevice->createTexture(dummyDesc);

    state.initialized = true;
}

MotionVectorOutput setupMotionVectorPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle depthInput,
    VirtualResourceHandle visId,
    VirtualResourceHandle visDepth,
    VirtualResourceHandle motionVectors,
    const Fmatrix& invViewProj,
    const Fmatrix& prevViewProj,
    bool motionValid,
    u32 width, u32 height,
    MotionVectorPassState& state)
{
    InitializeResources(device, state);

    if (!state.pipeline)
        return {};

    const bool inPlace = motionVectors.is_valid();
    VirtualResourceHandle mvHandle = motionVectors;
    if (!inPlace) {
        ResourceDesc mvDesc;
        mvDesc.type = ResourceDesc::Type::Texture2D;
        mvDesc.debugName = "rt_MotionVectors";
        mvDesc.width = width;
        mvDesc.height = height;
        mvDesc.format = nvrhi::Format::RG16_FLOAT;
        mvDesc.isUAV = true;
        mvDesc.isTransient = true;
        mvHandle = fg.CreateTexture("rt_MotionVectors", mvDesc);
    }
    const bool hasVis = visId.is_valid() && visDepth.is_valid();

    struct PassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle visId;
        VirtualResourceHandle visDepth;
        VirtualResourceHandle motionVectors;
        fg::RenderDevice* device;
        MotionVectorPassState* state;
        Fmatrix invViewProj;
        Fmatrix prevViewProj;
        bool hasVis;
        bool motionValid;
        u32 width, height;
    };

    auto& passData = fg.addCallbackPass<PassData>(
        "Motion Vectors",
        [&, mvHandle, inPlace, hasVis](FrameGraph& builder, PassHandle pass, PassData& data) {
            RenderPassBuilder pb(builder, pass);
            data.depth = pb.read(depthInput, ResourceState::ShaderResource);
            if (hasVis) {
                data.visId = pb.read(visId, ResourceState::ShaderResource);
                data.visDepth = pb.read(visDepth, ResourceState::ShaderResource);
            }
            data.motionVectors = inPlace ? pb.readWrite(mvHandle, ResourceState::UnorderedAccess) : pb.write(mvHandle, ResourceState::UnorderedAccess);
            data.device = device;
            data.state = &state;
            data.invViewProj = invViewProj;
            data.prevViewProj = prevViewProj;
            data.hasVis = hasVis;
            data.motionValid = motionValid;
            data.width = width;
            data.height = height;
        },
        [](const PassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* mvTex = fg.GetPhysicalTexture(data.motionVectors);
            if (!depthTex || !mvTex) return;
            nvrhi::ITexture* visTex = data.hasVis ? fg.GetPhysicalTexture(data.visId) : nullptr;
            nvrhi::ITexture* visDepthTex = data.hasVis ? fg.GetPhysicalTexture(data.visDepth) : nullptr;
            const bool hasVis = visTex && visDepthTex;

            struct {
                Fmatrix invViewProj;
                Fmatrix prevViewProj;
                float screenW, screenH;
                float invScreenW, invScreenH;
                u32 hasVis;
                u32 motionValid;
                u32 pad0, pad1;
            } cb;
            cb.invViewProj = data.invViewProj;
            cb.prevViewProj = data.prevViewProj;
            cb.screenW = (float)data.width;
            cb.screenH = (float)data.height;
            cb.invScreenW = 1.0f / data.width;
            cb.invScreenH = 1.0f / data.height;
            cb.hasVis = hasVis ? 1u : 0u;
            cb.motionValid = data.motionValid ? 1u : 0u;
            cb.pad0 = 0;
            cb.pad1 = 0;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            cmdList->writeBuffer(data.state->cb, &cb, sizeof(cb));

            auto* mvRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("restir_motion_vectors", ".cs");
            BindingSetBuilder bsb(*mvRefl, nvDevice, "MotionVector");
            bsb.ConstantBuffer("MotionVectorParams", data.state->cb)
               .Texture("t_Depth", depthTex)
               .Texture("t_VisID", hasVis ? visTex : data.state->dummyVisId.Get())
               .Texture("t_VisDepth", hasVis ? visDepthTex : depthTex)
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
