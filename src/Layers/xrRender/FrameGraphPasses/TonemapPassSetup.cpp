// xrRender/FrameGraphPasses/TonemapPassSetup.cpp
#include "stdafx.h"
#include "TonemapPassSetup.h"
#include "ExposurePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::fg::passes {

void InitializeTonemapPass(nvrhi::IDevice* device, TonemapPassState& state) {
    if (state.initialized || !device) return;

    nvrhi::TextureDesc texDesc;
    texDesc.debugName = "FallbackExposure";
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = nvrhi::Format::R32_FLOAT;
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;

    state.fallbackExposureTexture = device->createTexture(texDesc);

    nvrhi::CommandListHandle cmdList = device->createCommandList();
    cmdList->open();
    float defaultExposure = 1.0f;
    cmdList->writeTexture(state.fallbackExposureTexture, 0, 0, &defaultExposure, sizeof(float));
    cmdList->close();
    device->executeCommandList(cmdList);

    if (GEnv.FrameGraphRenderer->GetShaderLoader()) {
        auto vsResult = GEnv.FrameGraphRenderer->GetShaderLoader()->LoadVertexShader("tonemap");
        auto psResult = GEnv.FrameGraphRenderer->GetShaderLoader()->LoadPixelShader("tonemap");
        if (vsResult.handle && psResult.handle) {
            auto& cache = framegraph::GetPassResourceCache();

            state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TonemapPass", *vsResult.reflection, *psResult.reflection, device);

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
                fbInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);

                state.pipeline = cache.GetOrCreatePipeline("TonemapPass", pipeDesc, fbInfo, device);
            }
        }
    }

    state.initialized = true;
}

void ShutdownTonemapPass(TonemapPassState& state) {
    state.fallbackExposureTexture = nullptr;
    state.pipeline = nullptr;
    state.bindingLayout = nullptr;
    state.initialized = false;
}

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    TonemapPassState& tonemapState,
    const ExposurePassState* exposureState)
{
    using namespace framegraph;

    if (device && device->GetNVRHIDevice())
        InitializeTonemapPass(device->GetNVRHIDevice(), tonemapState);

    bool hasExposure = exposureTexture.is_valid();
    bool hasOutputTarget = outputTarget.is_valid();

    auto& passData = fg.addCallbackPass<TonemapPassData>(
        "Tonemap",

        [hdrInput, exposureTexture, outputTarget, hasExposure, hasOutputTarget, width, height, &tonemapState, exposureState](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.hasExposure = hasExposure;
            data.passState = &tonemapState;
            data.exposurePassState = exposureState;

            data.hdrInput = passBuilder.read(hdrInput, ResourceState::ShaderResource);

            if (hasExposure) {
                data.exposureInput = passBuilder.read(exposureTexture, ResourceState::ShaderResource);
            }

            if (hasOutputTarget) {
                data.ldrOutput = passBuilder.write(outputTarget, ResourceState::RenderTarget);
            } else {
                framegraph::ResourceDesc ldrDesc;
                ldrDesc.type = framegraph::ResourceDesc::Type::Texture2D;
                ldrDesc.width = width;
                ldrDesc.height = height;
                ldrDesc.format = nvrhi::Format::RGBA8_UNORM;
                ldrDesc.isRenderTarget = true;
                ldrDesc.isTransient = false;
                ldrDesc.debugName = "rt_Final";

                data.ldrOutput = passBuilder.createTexture("rt_Final", ldrDesc);
            }
        },

        [](const TonemapPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            auto* ps = data.passState;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* device = cmdList->getDevice();

            if (!ps->pipeline || !ps->bindingLayout)
                return;

            auto* hdrTexture = fg.GetPhysicalTexture(data.hdrInput);
            auto* ldrTexture = fg.GetPhysicalTexture(data.ldrOutput);
            if (!hdrTexture || !ldrTexture)
                return;

            auto& cache = framegraph::GetPassResourceCache();

            auto* vsRefl = GEnv.FrameGraphRenderer->GetShaderLoader()->GetCachedReflection("tonemap", ".vs");
            auto* psRefl = GEnv.FrameGraphRenderer->GetShaderLoader()->GetCachedReflection("tonemap", ".ps");
            if (!vsRefl || !psRefl)
                return;

            framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, device, "Tonemap");
            bsb.Texture("t_hdr", hdrTexture);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->bindingLayout, device);
            if (!bindingSet)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(ldrTexture);
            auto framebuffer = cache.GetOrCreateFramebuffer("TonemapPass", fbDesc, device);

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

    return passData.ldrOutput;
}

} // namespace xray::render::fg::passes
