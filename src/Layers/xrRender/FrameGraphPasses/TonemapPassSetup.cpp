#include "stdafx.h"
#include "TonemapPassSetup.h"
#include "ExposurePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

static nvrhi::TextureHandle s_fallbackExposureTexture;
static bool s_tonemapPassInitialized = false;

void InitializeTonemapPass(nvrhi::IDevice* device) {
    if (s_tonemapPassInitialized || !device) return;

    nvrhi::TextureDesc texDesc;
    texDesc.debugName = "FallbackExposure";
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = nvrhi::Format::R32_FLOAT;
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;

    s_fallbackExposureTexture = device->createTexture(texDesc);

    nvrhi::CommandListHandle cmdList = device->createCommandList();
    cmdList->open();
    float defaultExposure = 1.0f;
    cmdList->writeTexture(s_fallbackExposureTexture, 0, 0, &defaultExposure, sizeof(float));
    cmdList->close();
    device->executeCommandList(cmdList);

    s_tonemapPassInitialized = true;
}

void ShutdownTonemapPass() {
    s_fallbackExposureTexture = nullptr;
    s_tonemapPassInitialized = false;
}

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hdrInput,
    framegraph::VirtualResourceHandle exposureTexture,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct TonemapPassData {
        VirtualResourceHandle hdrInput;
        VirtualResourceHandle exposureInput;
        VirtualResourceHandle ldrOutput;
        bool hasExposure;
        u32 width;
        u32 height;
    };

    bool hasExposure = exposureTexture.is_valid();
    bool hasOutputTarget = outputTarget.is_valid();

    auto& passData = fg.addCallbackPass<TonemapPassData>(
        "Tonemap",

        [hdrInput, exposureTexture, outputTarget, hasExposure, hasOutputTarget, width, height](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.hasExposure = hasExposure;

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
            cmdList->beginMarker("Tonemap Pass");

            auto* hdrTexture = fg.GetPhysicalTexture(data.hdrInput);
            auto* ldrTexture = fg.GetPhysicalTexture(data.ldrOutput);

            auto* exposureTexture = data.hasExposure ? passes::GetExposureTexture() : nullptr;

            if (!hdrTexture || !ldrTexture) {
                Msg("! [TonemapPass] Failed to get textures");
                cmdList->endMarker();
                return;
            }

            if (!RImplementation.m_shaderLoader) {
                Msg("! [TonemapPass] ShaderLoader not initialized");
                cmdList->endMarker();
                return;
            }

            auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("tonemap");
            auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("tonemap");

            if (!vsResult.handle || !psResult.handle) {
                Msg("! [TonemapPass] Failed to load shaders");
                cmdList->endMarker();
                return;
            }

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Texture_SRV(1),
                nvrhi::BindingLayoutItem::Sampler(0)
            };

            auto bindingLayout = cache.GetOrCreateBindingLayout("TonemapPass", bindingLayoutDesc, device);

            if (!bindingLayout) {
                Msg("! [TonemapPass] Failed to create binding layout");
                cmdList->endMarker();
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
                cmdList->endMarker();
                return;
            }

            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cache.GetOrCreateSampler("TonemapPass", samplerDesc, device);

            if (!sampler) {
                Msg("! [TonemapPass] Failed to create sampler");
                cmdList->endMarker();
                return;
            }

            nvrhi::ITexture* exposureToUse = exposureTexture;
            if (!exposureToUse) {
                exposureToUse = s_fallbackExposureTexture.Get();
            }

            if (!exposureToUse) {
                Msg("! [TonemapPass] No exposure texture available (static fallback not initialized)");
                cmdList->endMarker();
                return;
            }

            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::Texture_SRV(0, hdrTexture),
                nvrhi::BindingSetItem::Texture_SRV(1, exposureToUse),
                nvrhi::BindingSetItem::Sampler(0, sampler)
            };

            auto bindingSet = cmdList->getDevice()->createBindingSet(bindingSetDesc, bindingLayout);

            if (!bindingSet) {
                Msg("! [TonemapPass] Failed to create binding set");
                cmdList->endMarker();
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
