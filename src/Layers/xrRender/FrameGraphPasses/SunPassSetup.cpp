// xrRender/FrameGraphPasses/SunPassSetup.cpp
#include "stdafx.h"
#include "SunPassSetup.h"
#include "ShaderConstants.h"
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

namespace RENDER_NAMESPACE {
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Sun vertex: position + color + UV
#pragma pack(push, 1)
struct SunVertex {
    Fvector3 position;
    u32 color;
    float u, v;
};
#pragma pack(pop)

// Static placeholder texture (1x1 white)
static nvrhi::TextureHandle s_sunPlaceholderTexture;
static bool s_sunPassInitialized = false;

void InitializeSunPass(ng::RenderDevice* device) {
    if (s_sunPassInitialized || !device) return;

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
    s_sunPlaceholderTexture = nvrhiDevice->createTexture(texDesc);

    // Initialize with white
    nvrhi::CommandListHandle cmdList = nvrhiDevice->createCommandList();
    cmdList->open();
    u32 white = 0xFFFFFFFF;
    cmdList->writeTexture(s_sunPlaceholderTexture, 0, 0, &white, sizeof(white));
    cmdList->close();
    nvrhiDevice->executeCommandList(cmdList);

    s_sunPassInitialized = true;
}

void ShutdownSunPass() {
    s_sunPlaceholderTexture = nullptr;
    s_sunPassInitialized = false;
}

framegraph::VirtualResourceHandle setupSunPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    CEnvironment* environment,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct SunPassData {
        VirtualResourceHandle colorOutput;
        ng::RenderDevice* device;
        CEnvironment* environment;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<SunPassData>(
        "Sun",

        // Setup lambda
        [colorInput, device, environment, width, height](
            FrameGraph& builder, PassHandle passHandle, SunPassData& data) {

            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.environment = environment;
            data.width = width;
            data.height = height;

            // Read-Write color (sun adds to existing sky)
            data.colorOutput = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);
        },

        // Execute lambda
        [](const SunPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            // Debug: Log that we're entering sun pass execute
            static bool s_logOnce = true;
            if (s_logOnce) {
                Msg("* [SunPass] Execute lambda called");
                s_logOnce = false;
            }

            // Get lens flare system
            CLensFlare* lensFlare = data.environment ? data.environment->eff_LensFlare : nullptr;
            if (!lensFlare) {
                static bool s_noLensFlare = true;
                if (s_noLensFlare) {
                    Msg("! [SunPass] No lens flare system (environment=%p)", data.environment);
                    s_noLensFlare = false;
                }
                return;
            }

            // Get current lens flare descriptor (updated by OnFrame)
            CLensFlareDescriptor* flareDesc = lensFlare->GetCurrent();
            if (!flareDesc) {
                static bool s_noFlareDesc = true;
                if (s_noFlareDesc) {
                    Msg("! [SunPass] No flare descriptor");
                    s_noFlareDesc = false;
                }
                return;
            }

            // Check if sun source should be rendered
            if (!flareDesc->m_Flags.is(CLensFlareDescriptor::flSource)) {
                static bool s_noSource = true;
                if (s_noSource) {
                    Msg("! [SunPass] flSource flag not set");
                    s_noSource = false;
                }
                return;
            }

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

            static bool s_sunRendering = true;
            if (s_sunRendering) {
                Msg("* [SunPass] Rendering sun (fDot=%.3f)", fDot);
                s_sunRendering = false;
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

            // Create vertex buffer
            nvrhi::BufferDesc vbDesc;
            vbDesc.byteSize = sizeof(vertices);
            vbDesc.debugName = "SunVertexBuffer";
            vbDesc.isVertexBuffer = true;
            vbDesc.isVolatile = false;
            vbDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
            vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
            auto vb = cmdList->getDevice()->createBuffer(vbDesc);
            cmdList->writeBuffer(vb, vertices, sizeof(vertices));

            // Index buffer for two triangles
            u16 indices[] = { 0, 1, 2, 2, 1, 3 };
            nvrhi::BufferDesc ibDesc;
            ibDesc.byteSize = sizeof(indices);
            ibDesc.debugName = "SunIndexBuffer";
            ibDesc.isIndexBuffer = true;
            ibDesc.isVolatile = false;
            ibDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
            ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
            auto ib = cmdList->getDevice()->createBuffer(ibDesc);
            cmdList->writeBuffer(ib, indices, sizeof(indices));

            // ═══════════════════════════════════════════════════════
            //  LOAD SUN SHADER
            // ═══════════════════════════════════════════════════════

            if (!RImplementation.m_shaderLoader) {
                Msg("! [SunPass] ShaderLoader not initialized");
                return;
            }

            auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("sun_forward");
            auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("sun_forward");

            if (!vsResult.handle || !psResult.handle) {
                Msg("! [SunPass] Failed to load sun shaders");
                return;
            }

            // ═══════════════════════════════════════════════════════
            //  CREATE PIPELINE (cached)
            // ═══════════════════════════════════════════════════════

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            // Input layout
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

            auto inputLayout = cache.GetOrCreateInputLayout(
                "SunPass", vertexAttribs, std::size(vertexAttribs), vsResult.handle, device);

            // Binding layout
            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms - volatile
                nvrhi::BindingLayoutItem::Texture_SRV(0),             // sun texture
                nvrhi::BindingLayoutItem::Sampler(0)                  // sampler
            };
            auto bindingLayout = cache.GetOrCreateBindingLayout("SunPass", bindingLayoutDesc, device);

            // Render state: additive blending, no depth write
            nvrhi::RenderState renderState;
            renderState.blendState.targets[0].setBlendEnable(true);
            renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::One);
            renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::One);
            renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
            renderState.depthStencilState.setDepthTestEnable(false);
            renderState.depthStencilState.setDepthWriteEnable(false);
            renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            // Get color RT
            nvrhi::ITexture* colorRT = fg.GetPhysicalTexture(data.colorOutput);
            if (!colorRT) {
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            auto framebuffer = cache.GetOrCreateFramebuffer("SunPass", fbDesc, device);

            nvrhi::FramebufferInfoEx fbInfo = framebuffer->getFramebufferInfo();

            nvrhi::GraphicsPipelineDesc pipelineDesc;
            pipelineDesc.inputLayout = inputLayout;
            pipelineDesc.VS = vsResult.handle;
            pipelineDesc.PS = psResult.handle;
            pipelineDesc.bindingLayouts = { bindingLayout };
            pipelineDesc.renderState = renderState;
            pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

            auto pipeline = cache.GetOrCreatePipeline("SunPass", pipelineDesc, fbInfo, device);

            if (!pipeline) {
                Msg("! [SunPass] Failed to create pipeline");
                return;
            }

            // ═══════════════════════════════════════════════════════
            //  CREATE CONSTANT BUFFER
            // ═══════════════════════════════════════════════════════

            // Use identity world matrix (vertices already in world space)
            DynamicTransforms dynamicCB = {};
            FillDynamicTransforms(dynamicCB, Fidentity);

            nvrhi::BufferDesc cbDesc;
            cbDesc.byteSize = sizeof(DynamicTransforms);
            cbDesc.isConstantBuffer = true;
            cbDesc.debugName = "SunDynamicTransforms";
            cbDesc.isVolatile = true;  // Per-frame data, no state tracking needed
            cbDesc.maxVersions = 16;
            auto dynamicCBBuffer = cmdList->getDevice()->createBuffer(cbDesc);
            cmdList->writeBuffer(dynamicCBBuffer, &dynamicCB, sizeof(dynamicCB));

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
                sunTex = s_sunPlaceholderTexture.Get();
            }

            // Create sampler (cached)
            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cache.GetOrCreateSampler("SunPass", samplerDesc, device);

            // Create binding set
            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynamicCBBuffer),
                nvrhi::BindingSetItem::Texture_SRV(0, sunTex),
                nvrhi::BindingSetItem::Sampler(0, sampler)
            };
            auto bindingSet = cmdList->getDevice()->createBindingSet(bindingSetDesc, bindingLayout);

            // ═══════════════════════════════════════════════════════
            //  RENDER SUN
            // ═══════════════════════════════════════════════════════

            nvrhi::GraphicsState state;
            state.pipeline = pipeline;
            state.framebuffer = framebuffer;
            state.bindings = { bindingSet };
            state.vertexBuffers = { { vb, 0, 0 } };
            state.indexBuffer = { ib, nvrhi::Format::R16_UINT, 0 };
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

} // namespace xray::render::RENDER_NAMESPACE::passes
