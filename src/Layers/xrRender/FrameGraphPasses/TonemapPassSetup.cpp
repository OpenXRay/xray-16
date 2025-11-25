#include "stdafx.h"
#include "TonemapPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

framegraph::VirtualResourceHandle setupTonemapPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hdrInput,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct TonemapPassData {
        VirtualResourceHandle hdrInput;
        VirtualResourceHandle ldrOutput;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<TonemapPassData>(
        "Tonemap",

        [hdrInput, width, height](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            data.hdrInput = passBuilder.read(hdrInput, ResourceState::ShaderResource);

            data.ldrOutput = passBuilder.createTexture2D(
                "rt_Final",
                width,
                height,
                nvrhi::Format::RGBA8_UNORM
            );
        },

        [](const TonemapPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Tonemap Pass");

            auto* hdrTexture = fg.GetPhysicalTexture(data.hdrInput);
            auto* ldrTexture = fg.GetPhysicalTexture(data.ldrOutput);

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

            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Sampler(0)
            };

            auto bindingLayout = cmdList->getDevice()->createBindingLayout(bindingLayoutDesc);

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

            auto pipeline = cmdList->getDevice()->createGraphicsPipeline(pipelineDesc, framebufferInfo);

            if (!pipeline) {
                Msg("! [TonemapPass] Failed to create pipeline");
                cmdList->endMarker();
                return;
            }

            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cmdList->getDevice()->createSampler(samplerDesc);

            if (!sampler) {
                Msg("! [TonemapPass] Failed to create sampler");
                cmdList->endMarker();
                return;
            }

            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::Texture_SRV(0, hdrTexture),
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
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            cmdList->open();
            cmdList->clearTextureFloat(ldrTexture, nvrhi::AllSubresources, nvrhi::Color(0.0f));

            nvrhi::GraphicsState state;
            state.pipeline = pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);

            cmdList->setGraphicsState(state);

            nvrhi::DrawArguments drawArgs;
            drawArgs.vertexCount = 3;
            cmdList->draw(drawArgs);

            cmdList->close();

            Msg("* [TonemapPass] Tonemap applied (HDR→LDR)");
            cmdList->endMarker();
        }
    );

    return passData.ldrOutput;
}

}
