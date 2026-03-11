#include "stdafx.h"
#include "DistortionApplyPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;

struct DistortionApplyData {
    VirtualResourceHandle sceneInput;
    VirtualResourceHandle distortionInput;
    VirtualResourceHandle worldPosInput;
    VirtualResourceHandle output;
    u32 width;
    u32 height;
    DistortionApplyPassState* passState;
};

void InitializeDistortionApplyPass(nvrhi::IDevice* device, DistortionApplyPassState& state) {
    if (state.initialized || !device) return;

    if (!RImplementation.m_shaderLoader)
        return;

    auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("fullscreen");
    auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("distortion_apply");
    if (!vsResult.handle || !psResult.handle) {
        state.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();

    state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "DistortionApply", *vsResult.reflection, *psResult.reflection, device);

    if (state.bindingLayout) {
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.setVertexShader(vsResult.handle);
        pipeDesc.setPixelShader(psResult.handle);
        pipeDesc.addBindingLayout(state.bindingLayout);
        pipeDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
        pipeDesc.renderState.blendState.targets[0].setBlendEnable(false);
        pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
        pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);
        pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);

        state.pipeline = cache.GetOrCreatePipeline("DistortionApply", pipeDesc, fbInfo, device);
    }
    state.initialized = true;
}

VirtualResourceHandle setupDistortionApplyPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle distortionRT,
    VirtualResourceHandle worldPos,
    u32 width,
    u32 height,
    DistortionApplyPassState& passState)
{
    if (device && device->GetNVRHIDevice())
        InitializeDistortionApplyPass(device->GetNVRHIDevice(), passState);

    ResourceDesc outputDesc;
    outputDesc.type = ResourceDesc::Type::Texture2D;
    outputDesc.width = width;
    outputDesc.height = height;
    outputDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outputDesc.isRenderTarget = true;
    outputDesc.isUAV = true;
    outputDesc.isTransient = true;
    outputDesc.debugName = "rt_DistortionApplied";
    VirtualResourceHandle outputHandle = fg.CreateTexture("rt_DistortionApplied", outputDesc);

    auto& passData = fg.addCallbackPass<DistortionApplyData>(
        "DistortionApply",

        [sceneColor, distortionRT, worldPos, outputHandle, width, height, &passState](FrameGraph& builder, PassHandle passHandle, DistortionApplyData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.width = width;
            data.height = height;
            data.passState = &passState;
            data.sceneInput = passBuilder.read(sceneColor, ResourceState::ShaderResource);
            data.distortionInput = passBuilder.read(distortionRT, ResourceState::ShaderResource);
            data.worldPosInput = passBuilder.read(worldPos, ResourceState::ShaderResource);
            data.output = passBuilder.write(outputHandle, ResourceState::RenderTarget);
        },

        [](const DistortionApplyData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* sceneTex = fg.GetPhysicalTexture(data.sceneInput);
            auto* distortTex = fg.GetPhysicalTexture(data.distortionInput);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPosInput);
            auto* outputTex = fg.GetPhysicalTexture(data.output);
            if (!sceneTex || !distortTex || !worldPosTex || !outputTex)
                return;

            auto* ps = data.passState;
            nvrhi::IDevice* device = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            if (!ps->pipeline || !ps->bindingLayout)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), ctx->GetDevice());

            auto* vsRefl = RImplementation.m_shaderLoader->GetCachedReflection("fullscreen", ".vs");
            auto* psRefl = RImplementation.m_shaderLoader->GetCachedReflection("distortion_apply", ".ps");
            if (!vsRefl || !psRefl)
                return;

            BindingSetBuilder bsb(*vsRefl, *psRefl, device, "DistortionApply");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB)
               .Texture("g_Snapshot", sceneTex)
               .Texture("g_Distortion", distortTex)
               .Texture("g_WorldPos", worldPosTex);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->bindingLayout, device);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputTex);
            auto framebuffer = cache.GetOrCreateFramebuffer("DistortionApply", fbDesc, device);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            nvrhi::GraphicsState state;
            state.pipeline = ps->pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);

            cmdList->setGraphicsState(state);
            cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
        }
    );

    return passData.output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
