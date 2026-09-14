#include "stdafx.h"
#include "PresentPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
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

framegraph::VirtualResourceHandle setupPresentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle sceneColor,
    framegraph::VirtualResourceHandle outputTarget,
    u32 width,
    u32 height,
    PresentPassState& state)
{
    using namespace framegraph;

    VERIFY(sceneColor.is_valid());
    VERIFY(outputTarget.is_valid());

    if (device)
        InitializePresentPass(device->GetNVRHIDevice(), fg.GetResourceDesc(outputTarget).format, state);

    struct PresentPassData {
        VirtualResourceHandle sceneColor;
        VirtualResourceHandle output;
        u32 width;
        u32 height;
        PresentPassState* passState;
    };

    auto& passData = fg.addCallbackPass<PresentPassData>(
        "Present",
        [sceneColor, outputTarget, width, height, &state](FrameGraph& builder, PassHandle passHandle, PresentPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.sceneColor = passBuilder.read(sceneColor, ResourceState::ShaderResource);
            data.output = passBuilder.write(outputTarget, ResourceState::RenderTarget);
            data.width = width;
            data.height = height;
            data.passState = &state;
        },
        [](const PresentPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
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
            auto framebuffer = cache.GetOrCreateFramebuffer("Present", fbDesc, device);

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
