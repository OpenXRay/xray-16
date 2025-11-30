#include "stdafx.h"
#include "SunPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
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

#pragma pack(push, 1)
struct SunVertex {
    Fvector3 position;
    u32 color;
    float u, v;
};
#pragma pack(pop)

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

        [colorInput, device, environment, width, height](
            FrameGraph& builder, PassHandle passHandle, SunPassData& data) {

            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.environment = environment;
            data.width = width;
            data.height = height;

            data.colorOutput = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);
        },

        [](const SunPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Sun Pass");

            CLensFlare* lensFlare = data.environment->eff_LensFlare;
            if (!lensFlare) {
                cmdList->endMarker();
                return;
            }

            CLensFlareDescriptor* flareDesc = lensFlare->GetCurrent();
            if (!flareDesc) {
                cmdList->endMarker();
                return;
            }

            if (!flareDesc->m_Flags.is(CLensFlareDescriptor::flSource)) {
                cmdList->endMarker();
                return;
            }

            const CEnvDescriptorMixer& env = data.environment->CurrentEnv;

            Fvector vSunDir;
            vSunDir.mul(env.sun_dir, -1.0f);
            vSunDir.normalize();

            float fDot = vSunDir.dotproduct(Device.vCameraDirection);

            if (fDot <= 0.01f) {
                cmdList->endMarker();
                return;
            }

            float fDistance = env.far_plane * 0.75f;

            Fvector vecCenter;
            vecCenter.mul(Device.vCameraDirection, fDistance);
            vecCenter.add(Device.vCameraPosition);

            Fvector vecLight;
            vecLight.set(vSunDir);
            vecLight.mul(fDistance / fDot);
            vecLight.add(Device.vCameraPosition);

            Fvector vecX, vecY;
            vecX.set(Device.vCameraRight);
            vecY.crossproduct(vecX, Device.vCameraDirection);

            float sunRadius = flareDesc->m_Source.fRadius;
            bool ignoreColor = flareDesc->m_Source.ignore_color;

            Fcolor sunColor;
            if (ignoreColor) {
                sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
            } else {
                sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);
            }

            float intensity = 2.0f;
            sunColor.r *= intensity;
            sunColor.g *= intensity;
            sunColor.b *= intensity;

            Fvector vecSx, vecSy;
            vecSx.mul(vecX, sunRadius * fDistance);
            vecSy.mul(vecY, sunRadius * fDistance);

            u32 c = sunColor.get();

            SunVertex vertices[4];
            vertices[0].position.x = vecLight.x + vecSx.x - vecSy.x;
            vertices[0].position.y = vecLight.y + vecSx.y - vecSy.y;
            vertices[0].position.z = vecLight.z + vecSx.z - vecSy.z;
            vertices[0].color = c;
            vertices[0].u = 0.0f;
            vertices[0].v = 0.0f;

            vertices[1].position.x = vecLight.x + vecSx.x + vecSy.x;
            vertices[1].position.y = vecLight.y + vecSx.y + vecSy.y;
            vertices[1].position.z = vecLight.z + vecSx.z + vecSy.z;
            vertices[1].color = c;
            vertices[1].u = 0.0f;
            vertices[1].v = 1.0f;

            vertices[2].position.x = vecLight.x - vecSx.x - vecSy.x;
            vertices[2].position.y = vecLight.y - vecSx.y - vecSy.y;
            vertices[2].position.z = vecLight.z - vecSx.z - vecSy.z;
            vertices[2].color = c;
            vertices[2].u = 1.0f;
            vertices[2].v = 0.0f;

            vertices[3].position.x = vecLight.x - vecSx.x + vecSy.x;
            vertices[3].position.y = vecLight.y - vecSx.y + vecSy.y;
            vertices[3].position.z = vecLight.z - vecSx.z + vecSy.z;
            vertices[3].color = c;
            vertices[3].u = 1.0f;
            vertices[3].v = 1.0f;

            nvrhi::BufferDesc vbDesc;
            vbDesc.byteSize = sizeof(vertices);
            vbDesc.debugName = "SunVertexBuffer";
            vbDesc.isVertexBuffer = true;
            vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
            auto vb = cmdList->getDevice()->createBuffer(vbDesc);
            cmdList->writeBuffer(vb, vertices, sizeof(vertices));

            u16 indices[] = { 0, 1, 2, 2, 1, 3 };
            nvrhi::BufferDesc ibDesc;
            ibDesc.byteSize = sizeof(indices);
            ibDesc.debugName = "SunIndexBuffer";
            ibDesc.isIndexBuffer = true;
            ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
            auto ib = cmdList->getDevice()->createBuffer(ibDesc);
            cmdList->writeBuffer(ib, indices, sizeof(indices));

            if (!RImplementation.m_shaderLoader) {
                Msg("! [SunPass] ShaderLoader not initialized");
                cmdList->endMarker();
                return;
            }

            auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("sun_forward");
            auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("sun_forward");

            if (!vsResult.handle || !psResult.handle) {
                Msg("! [SunPass] Failed to load sun shaders");
                cmdList->endMarker();
                return;
            }

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

            auto inputLayout = cmdList->getDevice()->createInputLayout(
                vertexAttribs, std::size(vertexAttribs), vsResult.handle);

            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::ConstantBuffer(0),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Sampler(0)
            };
            auto bindingLayout = cmdList->getDevice()->createBindingLayout(bindingLayoutDesc);

            nvrhi::RenderState renderState;
            renderState.blendState.targets[0].setBlendEnable(true);
            renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::One);
            renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::One);
            renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
            renderState.depthStencilState.setDepthTestEnable(false);
            renderState.depthStencilState.setDepthWriteEnable(false);
            renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            nvrhi::ITexture* colorRT = fg.GetPhysicalTexture(data.colorOutput);
            if (!colorRT) {
                cmdList->endMarker();
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            nvrhi::FramebufferInfoEx fbInfo = framebuffer->getFramebufferInfo();

            nvrhi::GraphicsPipelineDesc pipelineDesc;
            pipelineDesc.inputLayout = inputLayout;
            pipelineDesc.VS = vsResult.handle;
            pipelineDesc.PS = psResult.handle;
            pipelineDesc.bindingLayouts = { bindingLayout };
            pipelineDesc.renderState = renderState;
            pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

            auto pipeline = cmdList->getDevice()->createGraphicsPipeline(pipelineDesc, fbInfo);

            if (!pipeline) {
                Msg("! [SunPass] Failed to create pipeline");
                cmdList->endMarker();
                return;
            }

            DynamicTransforms dynamicCB = {};
            FillDynamicTransforms(dynamicCB, Fidentity);

            nvrhi::BufferDesc cbDesc;
            cbDesc.byteSize = sizeof(DynamicTransforms);
            cbDesc.isConstantBuffer = true;
            cbDesc.debugName = "SunDynamicTransforms";
            auto dynamicCBBuffer = cmdList->getDevice()->createBuffer(cbDesc);
            cmdList->writeBuffer(dynamicCBBuffer, &dynamicCB, sizeof(dynamicCB));

            nvrhi::ITexture* sunTex = nullptr;

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

            nvrhi::TextureHandle placeholderTex;
            if (!sunTex) {
                nvrhi::TextureDesc texDesc;
                texDesc.width = 1;
                texDesc.height = 1;
                texDesc.format = nvrhi::Format::RGBA8_UNORM;
                texDesc.debugName = "SunTexturePlaceholder";
                texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
                texDesc.keepInitialState = true;
                placeholderTex = cmdList->getDevice()->createTexture(texDesc);
                u32 white = 0xFFFFFFFF;
                cmdList->writeTexture(placeholderTex, 0, 0, &white, sizeof(white));
                sunTex = placeholderTex.Get();
            }

            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cmdList->getDevice()->createSampler(samplerDesc);

            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynamicCBBuffer),
                nvrhi::BindingSetItem::Texture_SRV(0, sunTex),
                nvrhi::BindingSetItem::Sampler(0, sampler)
            };
            auto bindingSet = cmdList->getDevice()->createBindingSet(bindingSetDesc, bindingLayout);

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

            cmdList->endMarker();
        }
    );

    return passData.colorOutput;
}

}
