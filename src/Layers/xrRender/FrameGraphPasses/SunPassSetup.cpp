// xrRender/FrameGraphPasses/SunPassSetup.cpp
#include "stdafx.h"
#include "SunPassSetup.h"
#include "PassVertexFormats.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/xr_efflensflare.h"

namespace fg {
    extern CRender RImplementation;
}

namespace xray::render::fg::passes {

void InitializeSunPass(fg::RenderDevice* device, SunPassState& state) {
    if (state.initialized || !device) return;

    auto* nvrhiDevice = device->GetNVRHIDevice();
    if (!nvrhiDevice) return;

    // Create 1x1 white placeholder texture
    nvrhi::TextureDesc texDesc;
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = nvrhi::Format::RGBA8_UNORM;
    texDesc.debugName = "SunTexturePlaceholder";
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;  // OK - static texture persists
    state.placeholderTexture = nvrhiDevice->createTexture(texDesc);

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = 4 * sizeof(SunVertex);
    vbDesc.debugName = "SunVertexBuffer";
    vbDesc.isVertexBuffer = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    state.vertexBuffer = nvrhiDevice->createBuffer(vbDesc);

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = 6 * sizeof(u16);
    ibDesc.debugName = "SunIndexBuffer";
    ibDesc.isIndexBuffer = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    state.indexBuffer = nvrhiDevice->createBuffer(ibDesc);

    nvrhi::CommandListHandle cmdList = nvrhiDevice->createCommandList();
    cmdList->open();
    u32 white = 0xFFFFFFFF;
    cmdList->writeTexture(state.placeholderTexture, 0, 0, &white, sizeof(white));
    u16 indices[] = { 0, 1, 2, 2, 1, 3 };
    cmdList->writeBuffer(state.indexBuffer, indices, sizeof(indices));
    cmdList->close();
    nvrhiDevice->executeCommandList(cmdList);

    if (!RImplementation.m_shaderLoader)
        return;

    auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("sun_forward");
    auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("sun_forward");

    if (!vsResult.handle || !psResult.handle)
        return;

    auto& cache = framegraph::GetPassResourceCache();

    state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection("SunPass", *vsResult.reflection, *psResult.reflection, nvrhiDevice);

    nvrhi::VertexAttributeDesc vertexAttribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(SunVertex, position))
            .setElementStride(sizeof(SunVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setOffset(offsetof(SunVertex, color))
            .setElementStride(sizeof(SunVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(SunVertex, u))
            .setElementStride(sizeof(SunVertex)),
    };

    state.inputLayout = cache.GetOrCreateInputLayout(
        "SunPass", vertexAttribs, std::size(vertexAttribs), vsResult.handle, nvrhiDevice);

    nvrhi::RenderState renderState;
    renderState.blendState.targets[0].setBlendEnable(true);
    renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::One);
    renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::One);
    renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
    renderState.depthStencilState.setDepthTestEnable(false);
    renderState.depthStencilState.setDepthWriteEnable(false);
    renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.inputLayout = state.inputLayout;
    pipelineDesc.VS = vsResult.handle;
    pipelineDesc.PS = psResult.handle;
    pipelineDesc.bindingLayouts = { state.bindingLayout };
    pipelineDesc.renderState = renderState;
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);

    state.pipeline = cache.GetOrCreatePipeline("SunPass", pipelineDesc, fbInfo, nvrhiDevice);

    state.initialized = true;
}

void ShutdownSunPass(SunPassState& state) {
    state.placeholderTexture = nullptr;
    state.vertexBuffer = nullptr;
    state.indexBuffer = nullptr;
    state.pipeline = nullptr;
    state.bindingLayout = nullptr;
    state.inputLayout = nullptr;
    state.initialized = false;
}

framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SunPassState& sunState)
{
    using namespace framegraph;

    InitializeSunPass(device, sunState);

    auto& passData = fg.addCallbackPass<SunPassData>(
        "Sun",

        // Setup lambda
        [colorInput, device, environment, width, height, &sunState](
            FrameGraph& builder, PassHandle passHandle, SunPassData& data) {

            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.environment = environment;
            data.width = width;
            data.height = height;
            data.passState = &sunState;

            // Read-Write color (sun adds to existing sky)
            data.colorOutput = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);
        },

        // Execute lambda
        [](const SunPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            CLensFlare* lensFlare = data.environment ? data.environment->eff_LensFlare : nullptr;
            if (!lensFlare)
                return;

            CLensFlareDescriptor* flareDesc = lensFlare->GetCurrent();
            if (!flareDesc)
                return;

            if (!flareDesc->m_Flags.is(CLensFlareDescriptor::flSource))
                return;

            // Get sun direction from environment
            const CEnvDescriptorMixer& env = data.environment->CurrentEnv;

            // Compute sun visibility (dot product with camera direction)
            Fvector vSunDir;
            vSunDir.mul(env.sun_dir, -1.0f);  // Negate to get direction toward sun
            vSunDir.normalize();

            float fDot = vSunDir.dotproduct(Device.vCameraDirection);

            // Sun behind camera?
            if (fDot <= 0.01f) {
                // Don't log this - it's normal when looking away from sun
                return;
            }

            // Calculate sun position on far plane
            float fDistance = env.far_plane * 0.75f;

            Fvector vecCenter;
            vecCenter.mul(Device.vCameraDirection, fDistance);
            vecCenter.add(Device.vCameraPosition);

            Fvector vecLight;
            vecLight.set(vSunDir);
            vecLight.mul(fDistance / fDot);
            vecLight.add(Device.vCameraPosition);

            // Camera-aligned billboard vectors
            Fvector vecX, vecY;
            vecX.set(Device.vCameraRight);
            vecY.crossproduct(vecX, Device.vCameraDirection);

            // Get sun settings from lens flare descriptor (already validated above)
            float sunRadius = flareDesc->m_Source.fRadius;
            bool ignoreColor = flareDesc->m_Source.ignore_color;

            // Sun color from environment (or white if ignore_color is set)
            Fcolor sunColor;
            if (ignoreColor) {
                sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
            } else {
                sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);
            }

            // Scale by intensity (HDR sun should be bright)
            float intensity = 2.0f;
            sunColor.r *= intensity;
            sunColor.g *= intensity;
            sunColor.b *= intensity;

            // Build sun quad vertices
            Fvector vecSx, vecSy;
            vecSx.mul(vecX, sunRadius * fDistance);
            vecSy.mul(vecY, sunRadius * fDistance);

            u32 c = sunColor.get();

            SunVertex vertices[4];
            // Top-left
            vertices[0].position.x = vecLight.x + vecSx.x - vecSy.x;
            vertices[0].position.y = vecLight.y + vecSx.y - vecSy.y;
            vertices[0].position.z = vecLight.z + vecSx.z - vecSy.z;
            vertices[0].color = c;
            vertices[0].u = 0.0f;
            vertices[0].v = 0.0f;

            // Top-right
            vertices[1].position.x = vecLight.x + vecSx.x + vecSy.x;
            vertices[1].position.y = vecLight.y + vecSx.y + vecSy.y;
            vertices[1].position.z = vecLight.z + vecSx.z + vecSy.z;
            vertices[1].color = c;
            vertices[1].u = 0.0f;
            vertices[1].v = 1.0f;

            // Bottom-left
            vertices[2].position.x = vecLight.x - vecSx.x - vecSy.x;
            vertices[2].position.y = vecLight.y - vecSx.y - vecSy.y;
            vertices[2].position.z = vecLight.z - vecSx.z - vecSy.z;
            vertices[2].color = c;
            vertices[2].u = 1.0f;
            vertices[2].v = 0.0f;

            // Bottom-right
            vertices[3].position.x = vecLight.x - vecSx.x + vecSy.x;
            vertices[3].position.y = vecLight.y - vecSx.y + vecSy.y;
            vertices[3].position.z = vecLight.z - vecSx.z + vecSy.z;
            vertices[3].color = c;
            vertices[3].u = 1.0f;
            vertices[3].v = 1.0f;

            cmdList->writeBuffer(data.passState->vertexBuffer, vertices, sizeof(vertices));

            if (!data.passState->pipeline)
                return;

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            nvrhi::ITexture* colorRT = fg.GetPhysicalTexture(data.colorOutput);
            if (!colorRT)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            auto framebuffer = cache.GetOrCreateFramebuffer("SunPass", fbDesc, device);

            // ═══════════════════════════════════════════════════════
            //  CREATE CONSTANT BUFFER
            // ═══════════════════════════════════════════════════════

            auto dynamicCBBuffer = framegraph::GetPassResourceCache().GetOrCreateVolatileCB(
                "Frame", "DynamicTransforms", sizeof(DynamicTransforms), data.device);

            // ═══════════════════════════════════════════════════════
            //  GET SUN TEXTURE
            // ═══════════════════════════════════════════════════════

            nvrhi::ITexture* sunTex = nullptr;

            // Load sun texture from lens flare descriptor (flareDesc is validated above)
            const shared_str& sunTexName = flareDesc->m_Source.texture;
            if (sunTexName.size()) {
                auto* resourceManager = data.device->GetFGResourceManager();
                if (resourceManager) {
                    resources::TextureManager* texManager = resourceManager->GetTextureManager();
                    if (texManager) {
                        auto handle = texManager->LoadTexture(sunTexName.c_str());
                        sunTex = texManager->GetNVRHITexture(handle);
                    }
                }
            }

            // Fallback to static placeholder if texture not available
            if (!sunTex) {
                sunTex = data.passState->placeholderTexture.Get();
            }

            auto* vsRefl = RImplementation.m_shaderLoader->GetCachedReflection("sun_forward", ".vs");
            auto* psRefl = RImplementation.m_shaderLoader->GetCachedReflection("sun_forward", ".ps");
            if (!vsRefl || !psRefl) return;
            framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, device, "Sun");
            bsb.ConstantBuffer("dynamic_transforms", dynamicCBBuffer)
               .Texture("s_sun", sunTex);
            auto bindingSetDesc = bsb.Build();
            auto bindingSet = cache.GetOrCreateBindingSet(bindingSetDesc, data.passState->bindingLayout, device);

            // ═══════════════════════════════════════════════════════
            //  RENDER SUN
            // ═══════════════════════════════════════════════════════

            nvrhi::GraphicsState state;
            state.pipeline = data.passState->pipeline;
            state.framebuffer = framebuffer;
            state.bindings = { bindingSet };
            state.vertexBuffers = { { data.passState->vertexBuffer, 0, 0 } };
            state.indexBuffer = { data.passState->indexBuffer, nvrhi::Format::R16_UINT, 0 };
            state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
                nvrhi::Viewport((float)data.width, (float)data.height));

            cmdList->setGraphicsState(state);

            nvrhi::DrawArguments args;
            args.vertexCount = 6;
            cmdList->drawIndexed(args);
        }
    );

    return passData.colorOutput;
}

} // namespace xray::render::fg::passes
