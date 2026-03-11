#include "stdafx.h"
#include "DecalPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Decals/DecalManager.h"

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;

struct DecalPassData {
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle sceneColor;
    VirtualResourceHandle worldPos;
    ng::RenderDevice* device;
    decals::DecalManager* decalMgr;
    DecalPassState* passState;
    u32 width, height;
};

static void InitializeDecalResources(ng::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, DecalPassState& state)
{
    if (state.initialized)
        return;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("decal_box");
    auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("decal_box");
    if (!vsResult.handle || !psResult.handle)
        return;

    state.vs = vsResult.handle;
    state.ps = psResult.handle;

    nvrhi::VertexAttributeDesc posAttr;
    posAttr.name = "POSITION";
    posAttr.format = nvrhi::Format::RGB32_FLOAT;
    posAttr.offset = 0;
    posAttr.bufferIndex = 0;
    posAttr.elementStride = sizeof(Fvector);
    state.inputLayout = nvDevice->createInputLayout(&posAttr, 1, state.vs);

    state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "Decal", *vsResult.reflection, *psResult.reflection, nvDevice);

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.setVertexShader(state.vs);
    pipeDesc.setPixelShader(state.ps);
    pipeDesc.setInputLayout(state.inputLayout);
    pipeDesc.addBindingLayout(state.bindingLayout);

    if (GEnv.Backend && GEnv.Backend->GetBindlessLayout())
        pipeDesc.addBindingLayout(GEnv.Backend->GetBindlessLayout());

    pipeDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
    pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);
    pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::Front);

    auto& blend = pipeDesc.renderState.blendState.targets[0];
    blend.setBlendEnable(true);
    blend.setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
    blend.setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
    blend.setBlendOp(nvrhi::BlendOp::Add);
    blend.setSrcBlendAlpha(nvrhi::BlendFactor::One);
    blend.setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);
    blend.setBlendOpAlpha(nvrhi::BlendOp::Add);

    state.pipeline = cache.GetOrCreatePipeline("Decal", pipeDesc, fbInfo, nvDevice);
    state.initialized = state.pipeline != nullptr;
}

DefaultOutputLayout setupDecalPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    const DefaultOutputLayout& inputs,
    decals::DecalManager* decalMgr,
    u32 width, u32 height,
    DecalPassState& state)
{
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    InitializeDecalResources(device, fbInfo, state);

    auto& passData = fg.addCallbackPass<DecalPassData>(
        "Decals",

        [&](FrameGraph& builder, PassHandle passHandle, DecalPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.device = device;
            data.decalMgr = decalMgr;
            data.passState = &state;
            data.width = width;
            data.height = height;
            data.depth = passBuilder.read(inputs.depth, ResourceState::DepthStencilRead);
            data.normal = passBuilder.read(inputs.normal, ResourceState::ShaderResource);
            data.sceneColor = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.worldPos = passBuilder.read(inputs.worldPos, ResourceState::ShaderResource);
        },

        [](const DecalPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();

            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* colorTex = fg.GetPhysicalTexture(data.sceneColor);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            if (!depthTex || !normalTex || !colorTex || !worldPosTex)
                return;

            data.decalMgr->Upload(ctx);

            if (data.decalMgr->GetActiveCount() == 0)
                return;

            auto& cache = GetPassResourceCache();

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTex);
            auto framebuffer = cache.GetOrCreateFramebuffer("Decal", fbDesc, nvDevice);

            if (!data.passState->initialized)
                return;

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);

            auto* vsReflection = RImplementation.m_shaderLoader->GetCachedReflection("decal_box", ".vs");
            auto* psReflection = RImplementation.m_shaderLoader->GetCachedReflection("decal_box", ".ps");

            auto* materialBuffer = MaterialBuffer::Instance().GetBuffer();

            framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Decal");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.BufferSRV("g_Decals", data.decalMgr->GetDecalBuffer());
            bsb.Texture("g_Depth", depthTex);
            bsb.Texture("g_WorldPos", worldPosTex);
            bsb.BufferSRV("g_Materials", materialBuffer);

            auto bindingSet = cache.GetOrCreateBindingSet(
                bsb.Build(), data.passState->bindingLayout, nvDevice);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            nvrhi::GraphicsState gfxState;
            gfxState.pipeline = data.passState->pipeline;
            gfxState.framebuffer = framebuffer;
            gfxState.viewport.addViewportAndScissorRect(viewport);
            gfxState.addBindingSet(bindingSet);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            if (bindlessTable)
                gfxState.addBindingSet(bindlessTable);

            gfxState.vertexBuffers = { { data.decalMgr->GetCubeVB(), 0, 0 } };
            gfxState.indexBuffer = { data.decalMgr->GetCubeIB(), nvrhi::Format::R16_UINT, 0 };

            cmdList->setGraphicsState(gfxState);

            nvrhi::DrawArguments args;
            args.vertexCount = decals::CUBE_INDEX_COUNT;
            args.instanceCount = data.decalMgr->GetActiveCount();
            cmdList->drawIndexed(args);
        }
    );

    DefaultOutputLayout output = inputs;
    output.albedo = passData.sceneColor;
    output.depth = passData.depth;
    output.normal = passData.normal;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
