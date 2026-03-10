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

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

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

    state.initialized = true;
}

void ShutdownTonemapPass(TonemapPassState& state) {
    state.fallbackExposureTexture = nullptr;
    state.initialized = false;
}

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    TonemapPassState& tonemapState,
    const ExposurePassState* exposureState)
{
    using namespace framegraph;

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
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            auto* hdrTexture = fg.GetPhysicalTexture(data.hdrInput);
            auto* ldrTexture = fg.GetPhysicalTexture(data.ldrOutput);

            auto* exposureTexture = (data.hasExposure && data.exposurePassState)
                ? passes::GetExposureTexture(*data.exposurePassState) : nullptr;

            if (!hdrTexture || !ldrTexture) {
                Msg("! [TonemapPass] Failed to get textures");
                return;
            }

            if (!RImplementation.m_shaderLoader) {
                Msg("! [TonemapPass] ShaderLoader not initialized");
                return;
            }

            auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("tonemap");
            auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("tonemap");

            if (!vsResult.handle || !psResult.handle) {
                Msg("! [TonemapPass] Failed to load shaders");
                return;
            }

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            auto bindingLayout = cache.GetOrCreateBindingLayoutFromReflection("TonemapPass", *vsResult.reflection, *psResult.reflection, device);

            if (!bindingLayout) {
                Msg("! [TonemapPass] Failed to create binding layout");
                return;
            }

            nvrhi::RenderState renderState;
            renderState.blendState.targets[0].setBlendEnable(false);
            renderState.depthStencilState.setDepthTestEnable(false);
            renderState.depthStencilState.setDepthWriteEnable(false);
            renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            nvrhi::GraphicsPipelineDesc pipelineDesc;
            pipelineDesc.setVertexShader(vsResult.handle);
            pipelineDesc.setPixelShader(psResult.handle);
            pipelineDesc.addBindingLayout(bindingLayout);
            pipelineDesc.setRenderState(renderState);
            pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

            nvrhi::FramebufferInfoEx framebufferInfo;
            framebufferInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);

            auto pipeline = cache.GetOrCreatePipeline("TonemapPass", pipelineDesc, framebufferInfo, device);

            if (!pipeline) {
                Msg("! [TonemapPass] Failed to create pipeline");
                return;
            }

            nvrhi::ITexture* exposureToUse = exposureTexture;
            if (!exposureToUse)
                exposureToUse = data.passState->fallbackExposureTexture.Get();

            if (!exposureToUse) {
                Msg("! [TonemapPass] No exposure texture available");
                return;
            }

            framegraph::BindingSetBuilder bsb(*vsResult.reflection, *psResult.reflection, device, "Tonemap");
            bsb.Texture("t_hdr", hdrTexture);

            auto bindingSet = device->createBindingSet(bsb.Build(), bindingLayout);

            if (!bindingSet) {
                Msg("! [TonemapPass] Failed to create binding set");
                return;
            }

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
            state.pipeline = pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);

            cmdList->setGraphicsState(state);

            nvrhi::DrawArguments drawArgs;
            drawArgs.vertexCount = 3;
            cmdList->draw(drawArgs);
        }
    );

    return passData.ldrOutput;
}

}
