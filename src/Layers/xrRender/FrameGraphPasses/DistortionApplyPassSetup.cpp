#include "stdafx.h"
#include "DistortionApplyPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;

struct DistortionApplyData {
    VirtualResourceHandle snapshotInput;
    VirtualResourceHandle distortionInput;
    VirtualResourceHandle worldPosInput;
    VirtualResourceHandle output;
    u32 width;
    u32 height;
};

VirtualResourceHandle setupDistortionApplyPass(
    FrameGraph& fg,
    VirtualResourceHandle sceneSnapshot,
    VirtualResourceHandle distortionRT,
    VirtualResourceHandle worldPos,
    u32 width,
    u32 height)
{
    ResourceDesc outputDesc;
    outputDesc.type = ResourceDesc::Type::Texture2D;
    outputDesc.width = width;
    outputDesc.height = height;
    outputDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outputDesc.isRenderTarget = true;
    outputDesc.isTransient = true;
    outputDesc.debugName = "rt_DistortionApplied";
    VirtualResourceHandle outputHandle = fg.CreateTexture("rt_DistortionApplied", outputDesc);

    auto& passData = fg.addCallbackPass<DistortionApplyData>(
        "DistortionApply",

        [sceneSnapshot, distortionRT, worldPos, outputHandle, width, height](FrameGraph& builder, PassHandle passHandle, DistortionApplyData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.width = width;
            data.height = height;
            data.snapshotInput = passBuilder.read(sceneSnapshot, ResourceState::ShaderResource);
            data.distortionInput = passBuilder.read(distortionRT, ResourceState::ShaderResource);
            data.worldPosInput = passBuilder.read(worldPos, ResourceState::ShaderResource);
            data.output = passBuilder.write(outputHandle, ResourceState::RenderTarget);
        },

        [](const DistortionApplyData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* snapshotTex = fg.GetPhysicalTexture(data.snapshotInput);
            auto* distortTex = fg.GetPhysicalTexture(data.distortionInput);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPosInput);
            auto* outputTex = fg.GetPhysicalTexture(data.output);
            if (!snapshotTex || !distortTex || !worldPosTex || !outputTex)
                return;

            if (!RImplementation.m_shaderLoader)
                return;

            auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("fullscreen");
            auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("distortion_apply");
            if (!vsResult.handle || !psResult.handle)
                return;

            auto& cache = GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            nvrhi::BindingLayoutDesc globalsLayoutDesc;
            globalsLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
            globalsLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
            };
            auto globalsLayout = cache.GetOrCreateBindingLayout("DistortionApply_globals", globalsLayoutDesc, device);

            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Pixel;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Texture_SRV(1),
                nvrhi::BindingLayoutItem::Texture_SRV(2),
                nvrhi::BindingLayoutItem::Sampler(0),
            };
            auto layout = cache.GetOrCreateBindingLayout("DistortionApply", layoutDesc, device);

            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.setVertexShader(vsResult.handle);
            pipeDesc.setPixelShader(psResult.handle);
            pipeDesc.addBindingLayout(globalsLayout);
            pipeDesc.addBindingLayout(layout);
            pipeDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
            pipeDesc.renderState.blendState.targets[0].setBlendEnable(false);
            pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
            pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);
            pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputTex);
            auto framebuffer = cache.GetOrCreateFramebuffer("DistortionApply", fbDesc, device);
            auto fbInfo = framebuffer->getFramebufferInfo();

            auto pipeline = cache.GetOrCreatePipeline("DistortionApply", pipeDesc, fbInfo, device);
            if (!pipeline)
                return;

            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cache.GetOrCreateSampler("DistortionApply", samplerDesc, device);

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("DistortionApply", "globals", sizeof(StaticGlobals), 16, device);
            auto staticGlobals = BuildStaticGlobals();
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            nvrhi::BindingSetDesc globalsBindDesc;
            globalsBindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
            };
            auto globalsBindingSet = cache.GetOrCreateBindingSet(globalsBindDesc, globalsLayout, device);

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::Texture_SRV(0, snapshotTex),
                nvrhi::BindingSetItem::Texture_SRV(1, distortTex),
                nvrhi::BindingSetItem::Texture_SRV(2, worldPosTex),
                nvrhi::BindingSetItem::Sampler(0, sampler),
            };
            auto bindingSet = device->createBindingSet(bindDesc, layout);

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
            state.addBindingSet(globalsBindingSet);
            state.addBindingSet(bindingSet);

            cmdList->setGraphicsState(state);
            cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
        }
    );

    return passData.output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
