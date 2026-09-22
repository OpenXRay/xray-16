#include "stdafx.h"
#include "PresentPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg::passes {

static void InitializePresentPass(nvrhi::IDevice* device, nvrhi::Format outputFormat, PresentPassState& state) {
    if (state.initialized || !device) return;

    if (auto* loader = GEnv.Render->GetShaderLoader()) {
        auto vsResult = loader->LoadVertexShader("fullscreen");
        auto psResult = loader->LoadPixelShader("present");
        if (vsResult.handle && psResult.handle) {
            auto& cache = framegraph::GetPassResourceCache();
            state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "Present", *vsResult.reflection, *psResult.reflection, device);

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
                fbInfo.addColorFormat(outputFormat);
                state.pipeline = cache.GetOrCreatePipeline("Present", pipeDesc, fbInfo, device);
            }
        }
    }

    state.initialized = true;
}

framegraph::VirtualResourceHandle setupLightingFailurePass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle sceneColor,
    LightingFrameState& lighting)
{
    using namespace framegraph;

    if (!lighting.opaqueScheduled || lighting.scheduled == LightingMode::Raster || !sceneColor.is_valid())
        return sceneColor;

    auto& passData = fg.addCallbackPass<LightingFailurePassData>(
        "LightingFailure",
        [sceneColor, &lighting](FrameGraph& builder, PassHandle passHandle, LightingFailurePassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.sceneColor = passBuilder.readWrite(sceneColor, ResourceState::RenderTarget);
            passBuilder.sideEffects();
            data.lighting = &lighting;
        },
        [](const LightingFailurePassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            auto* lightingState = data.lighting;
            if (!lightingState || (lightingState->recorded && !lightingState->frameFailed))
                return;

            if (!lightingState->frameFailed)
                lightingState->Fail(LightingFallback::RecordingUnavailable);

            Msg("! [LightingFailure] ray traced lighting recorded no world color (%s); clearing the world to black",
                LightingFallbackName(lightingState->fallback));

            nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
            auto* sceneTarget = fg.GetPhysicalTexture(data.sceneColor);
            if (!cmdList || !sceneTarget)
                return;

            cmdList->clearTextureFloat(sceneTarget, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
            lightingState->failureCleared = true;
        }
    );

    return passData.sceneColor;
}

framegraph::VirtualResourceHandle setupPresentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    PresentPassState& state,
    LightingFrameState* lighting)
{
    using namespace framegraph;

    VERIFY(sceneColor.is_valid());
    VERIFY(outputTarget.is_valid());

    if (device)
        InitializePresentPass(device->GetNVRHIDevice(), fg.GetResourceDesc(outputTarget).format, state);

    auto& passData = fg.addCallbackPass<PresentPassData>(
        "Present",
        [sceneColor, outputTarget, width, height, &state, lighting](FrameGraph& builder, PassHandle passHandle, PresentPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.sceneColor = passBuilder.read(sceneColor, ResourceState::ShaderResource);
            data.output = passBuilder.write(outputTarget, ResourceState::RenderTarget);
            data.width = width;
            data.height = height;
            data.passState = &state;
            data.lighting = lighting;
        },
        [](const PresentPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (data.lighting && data.lighting->frameFailed)
            {
                nvrhi::ICommandList* cmdList = ctx ? ctx->GetCommandList() : nullptr;
                auto* target = fg.GetPhysicalTexture(data.output);
                if (cmdList && target)
                    cmdList->clearTextureFloat(target, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
                if (!data.lighting->failureCleared)
                    return;
            }

            auto* ps = data.passState;
            if (!ps->pipeline || !ps->bindingLayout)
                return;

            auto* sourceTexture = fg.GetPhysicalTexture(data.sceneColor);
            auto* outputTexture = fg.GetPhysicalTexture(data.output);
            if (!sourceTexture || !outputTexture)
                return;

            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = loader->GetCachedReflection("fullscreen", ".vs");
            auto* psRefl = loader->GetCachedReflection("present", ".ps");
            if (!vsRefl || !psRefl)
                return;

            auto* cmdList = ctx->GetCommandList();
            auto* device = cmdList->getDevice();
            auto& cache = framegraph::GetPassResourceCache();
            BindingSetBuilder bsb(*vsRefl, *psRefl, device, "Present");
            bsb.Texture("t_scene", sourceTexture);
            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), ps->bindingLayout, device);
            if (!bindingSet)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputTexture);
            auto framebuffer = cache.GetOrCreateFramebuffer(fbDesc, device);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            nvrhi::GraphicsState graphicsState;
            graphicsState.pipeline = ps->pipeline;
            graphicsState.framebuffer = framebuffer;
            graphicsState.viewport.addViewportAndScissorRect(viewport);
            graphicsState.addBindingSet(bindingSet);

            cmdList->setGraphicsState(graphicsState);
            cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
        }
    );

    return passData.output;
}

}
