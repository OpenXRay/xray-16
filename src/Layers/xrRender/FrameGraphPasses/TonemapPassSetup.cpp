// xrRender/FrameGraphPasses/TonemapPassSetup.cpp
#include "stdafx.h"
#include "TonemapPassSetup.h"
#include "ExposurePassSetup.h"  // For GetExposureTexture()
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

// Static fallback exposure texture (1x1, value = 1.0)
static nvrhi::TextureHandle s_fallbackExposureTexture;
static bool s_tonemapPassInitialized = false;

void InitializeTonemapPass(nvrhi::IDevice* device) {
    if (s_tonemapPassInitialized || !device) return;

    // Create 1x1 R32_FLOAT texture with default exposure = 1.0
    nvrhi::TextureDesc texDesc;
    texDesc.debugName = "FallbackExposure";
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = nvrhi::Format::R32_FLOAT;
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;  // OK - static texture persists

    s_fallbackExposureTexture = device->createTexture(texDesc);

    // Initialize with default exposure = 1.0
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

    // Check if exposure texture is valid
    bool hasExposure = exposureTexture.is_valid();
    bool hasOutputTarget = outputTarget.is_valid();

    auto& passData = fg.addCallbackPass<TonemapPassData>(
        "Tonemap",

        // Setup lambda
        [hdrInput, exposureTexture, outputTarget, hasExposure, hasOutputTarget, width, height](FrameGraph& builder, PassHandle passHandle, TonemapPassData& data) {
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

            // Use provided output target (backbuffer) or create internal rt_Final
            if (hasOutputTarget) {
                // Write directly to imported backbuffer (Frostbite pattern)
                data.ldrOutput = passBuilder.write(outputTarget, ResourceState::RenderTarget);
            } else {
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
            }
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
            //  CREATE PIPELINE STATE (cached)
            // ═══════════════════════════════════════════════════

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            // Create binding layout
            // t0: HDR texture, t1: Exposure texture (1x1), s0: Linear sampler
            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),  // t0: HDR texture
                nvrhi::BindingLayoutItem::Texture_SRV(1),  // t1: Exposure texture (1x1 R32_FLOAT)
                nvrhi::BindingLayoutItem::Sampler(0)       // s0: Linear sampler
            };

            auto bindingLayout = cache.GetOrCreateBindingLayout("TonemapPass", bindingLayoutDesc, device);

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

            auto pipeline = cache.GetOrCreatePipeline("TonemapPass", pipelineDesc, framebufferInfo, device);

            if (!pipeline) {
                Msg("! [TonemapPass] Failed to create pipeline");
                cmdList->endMarker();
                return;
            }

            // ═══════════════════════════════════════════════════
            //  CREATE SAMPLER (cached)
            // ═══════════════════════════════════════════════════
            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);  // Linear filtering
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cache.GetOrCreateSampler("TonemapPass", samplerDesc, device);

            if (!sampler) {
                Msg("! [TonemapPass] Failed to create sampler");
                cmdList->endMarker();
                return;
            }

            // ═══════════════════════════════════════════════════
            //  GET EXPOSURE TEXTURE (use static fallback if not provided)
            // ═══════════════════════════════════════════════════
            nvrhi::ITexture* exposureToUse = exposureTexture;
            if (!exposureToUse) {
                exposureToUse = s_fallbackExposureTexture.Get();
            }

            if (!exposureToUse) {
                Msg("! [TonemapPass] No exposure texture available (static fallback not initialized)");
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

            // Create framebuffer (cached)
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(ldrTexture);
            auto framebuffer = cache.GetOrCreateFramebuffer("TonemapPass", fbDesc, device);

            // Setup viewport
            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            // Note: Command list is already open from FrameGraph::Execute()

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

            // Note: Command list is closed by FrameGraph::Execute()
            // cmdList->endMarker();  // Disabled - NVRHI D3D12 doesn't implement debug markers
        }
    );

    return passData.ldrOutput;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
