// xrRender/FrameGraphPasses/TonemapPassSetup.cpp
#include "stdafx.h"
#include "TonemapPassSetup.h"
#include "ExposurePassSetup.h"  // For GetExposureTexture()
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
    framegraph::VirtualResourceHandle exposureTexture,
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

    // Check if exposure texture is valid
    bool hasExposure = exposureTexture.is_valid();

    auto& passData = fg.addCallbackPass<TonemapPassData>(
        "Tonemap",

        // Setup lambda
        [hdrInput, exposureTexture, hasExposure, width, height](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.hasExposure = hasExposure;

            // Read HDR input
            data.hdrInput = passBuilder.read(hdrInput, ResourceState::ShaderResource);

            // Read exposure texture if available
            if (hasExposure) {
                data.exposureInput = passBuilder.read(exposureTexture, ResourceState::ShaderResource);
            }

            // Create LDR output (RGBA8_UNORM for standard displays)
            // NOTE: Must be non-transient (persistent) to be a terminal pass and keep render chain alive
            framegraph::ResourceDesc ldrDesc;
            ldrDesc.type = framegraph::ResourceDesc::Type::Texture2D;
            ldrDesc.width = width;
            ldrDesc.height = height;
            ldrDesc.format = nvrhi::Format::RGBA8_UNORM;
            ldrDesc.isRenderTarget = true;
            ldrDesc.isTransient = false;  // CRITICAL: Makes this a terminal pass
            ldrDesc.debugName = "rt_Final";

            data.ldrOutput = passBuilder.createTexture("rt_Final", ldrDesc);
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

            // Get exposure texture directly from ExposurePass (bypasses framegraph state issues)
            auto* exposureTexture = data.hasExposure ? passes::GetExposureTexture() : nullptr;

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

            // Create binding layout
            // t0: HDR texture, t1: Exposure texture (1x1), s0: Linear sampler
            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),  // t0: HDR texture
                nvrhi::BindingLayoutItem::Texture_SRV(1),  // t1: Exposure texture (1x1 R32_FLOAT)
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
            //  CREATE FALLBACK EXPOSURE TEXTURE (if not provided)
            // ═══════════════════════════════════════════════════
            nvrhi::ITexture* exposureToUse = exposureTexture;
            nvrhi::TextureHandle fallbackExposure;

            if (!exposureToUse) {
                // Create a 1x1 texture with default exposure = 1.0
                nvrhi::TextureDesc texDesc;
                texDesc.debugName = "FallbackExposure";
                texDesc.width = 1;
                texDesc.height = 1;
                texDesc.format = nvrhi::Format::R32_FLOAT;
                texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
                texDesc.keepInitialState = true;

                fallbackExposure = cmdList->getDevice()->createTexture(texDesc);
                if (fallbackExposure) {
                    float defaultExposure = 1.0f;
                    cmdList->writeTexture(fallbackExposure, 0, 0, &defaultExposure, sizeof(float));
                    exposureToUse = fallbackExposure.Get();
                }
            }

            if (!exposureToUse) {
                Msg("! [TonemapPass] Failed to create fallback exposure texture");
                cmdList->endMarker();
                return;
            }

            // ═══════════════════════════════════════════════════
            //  CREATE BINDING SET
            // ═══════════════════════════════════════════════════
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

            // Begin render pass (no clear needed - fullscreen triangle covers everything)
            cmdList->open();

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

            cmdList->endMarker();
        }
    );

    return passData.ldrOutput;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
