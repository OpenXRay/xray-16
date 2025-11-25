// xrRender/FrameGraphPasses/TonemapPassSetup.cpp
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

        // Setup lambda
        [hdrInput, width, height](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;

            // Read HDR input
            data.hdrInput = passBuilder.read(hdrInput, ResourceState::ShaderResource);

            // Create LDR output (RGBA8_UNORM for standard displays)
            data.ldrOutput = passBuilder.createTexture2D(
                "rt_Final",
                width,
                height,
                nvrhi::Format::RGBA8_UNORM
            );
        },

        // Execute lambda
        [](const TonemapPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Tonemap Pass");

            // Get physical resources
            auto* hdrTexture = fg.GetPhysicalTexture(data.hdrInput);
            auto* ldrTexture = fg.GetPhysicalTexture(data.ldrOutput);

            if (!hdrTexture || !ldrTexture) {
                Msg("! [TonemapPass] Failed to get textures");
                cmdList->endMarker();
                return;
            }

            // ═══════════════════════════════════════════════════
            //  LOAD SHADERS USING RImplementation.m_shaderLoader
            // ═══════════════════════════════════════════════════
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

            // ═══════════════════════════════════════════════════
            //  CREATE PIPELINE STATE
            // ═══════════════════════════════════════════════════

            // Create binding layout (texture + sampler, no constant buffer)
            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),  // t0: HDR texture
                nvrhi::BindingLayoutItem::Sampler(0)       // s0: Linear sampler
            };

            auto bindingLayout = cmdList->getDevice()->createBindingLayout(bindingLayoutDesc);

            if (!bindingLayout) {
                Msg("! [TonemapPass] Failed to create binding layout");
                cmdList->endMarker();
                return;
            }

            // Create render state (no blending, no depth test)
            nvrhi::RenderState renderState;
            renderState.blendState.targets[0].setBlendEnable(false);
            renderState.depthStencilState.setDepthTestEnable(false);
            renderState.depthStencilState.setDepthWriteEnable(false);
            renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            // Create pipeline
            nvrhi::GraphicsPipelineDesc pipelineDesc;
            pipelineDesc.setVertexShader(vsResult.handle);
            pipelineDesc.setPixelShader(psResult.handle);
            pipelineDesc.addBindingLayout(bindingLayout);
            pipelineDesc.setRenderState(renderState);
            pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

            // Framebuffer format (LDR output)
            nvrhi::FramebufferInfoEx framebufferInfo;
            framebufferInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);

            auto pipeline = cmdList->getDevice()->createGraphicsPipeline(pipelineDesc, framebufferInfo);

            if (!pipeline) {
                Msg("! [TonemapPass] Failed to create pipeline");
                cmdList->endMarker();
                return;
            }

            // ═══════════════════════════════════════════════════
            //  CREATE SAMPLER
            // ═══════════════════════════════════════════════════
            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);  // Linear filtering
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cmdList->getDevice()->createSampler(samplerDesc);

            if (!sampler) {
                Msg("! [TonemapPass] Failed to create sampler");
                cmdList->endMarker();
                return;
            }

            // ═══════════════════════════════════════════════════
            //  CREATE BINDING SET
            // ═══════════════════════════════════════════════════
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

            // ═══════════════════════════════════════════════════
            //  RENDER FULLSCREEN TRIANGLE
            // ═══════════════════════════════════════════════════

            // Create framebuffer
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(ldrTexture);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            // Setup viewport
            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            // Begin render pass
            cmdList->open();
            cmdList->clearTextureFloat(ldrTexture, nvrhi::AllSubresources, nvrhi::Color(0.0f));

            nvrhi::GraphicsState state;
            state.pipeline = pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);

            cmdList->setGraphicsState(state);

            // Draw fullscreen triangle (3 vertices, no vertex buffer, shader generates positions from SV_VertexID)
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

} // namespace xray::render::RENDER_NAMESPACE::passes
