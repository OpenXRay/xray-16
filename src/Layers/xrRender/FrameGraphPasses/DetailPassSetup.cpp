// DetailPassSetup.cpp - Framegraph pass for detail objects (grass, etc.)
#include "stdafx.h"
#include "PassCommon.h"
#include "DetailPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::fg
{
    extern int ps_r__detail_gpu;
}

namespace xray::render::fg::passes
{
using namespace framegraph;

DefaultOutputLayout setupDetailPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    const DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    xray::profiler::GPUProfiler* gpuProfiler
)
{
    if (detailManager && !detailManager->decalGraphicsPipeline)
    {
        nvrhi::FramebufferInfo fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        detailManager->CreateGraphicsPipeline(device, fbInfo);
    }

    auto& passData = fg.addCallbackPass<DetailPassData>(
        "DetailDraw",
        [&, width, height, gpuProfiler](
            FrameGraph& builder, PassHandle passHandle, DetailPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.detailManager = detailManager;
            data.gpuProfiler = gpuProfiler;

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);
            data.outputColor = passBuilder.write(forwardInputs.albedo);
            data.outputNormal = passBuilder.readWrite(forwardInputs.normal, ResourceState::RenderTarget);
            if (forwardInputs.baseColor.is_valid())
                data.baseColor = passBuilder.readWrite(forwardInputs.baseColor, ResourceState::RenderTarget);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = data.outputNormal;
            data.outputs.baseColor = data.baseColor;
            data.outputs.depth = data.depth;
        },
        [](const DetailPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("DetailPass", 10);

            auto* dm = data.detailManager;
            if (!dm)
                return;

            if (!psDeviceFlags.is(rsDrawDetails))
                return;

            if (!dm->instanceGenPipeline || !dm->slotDataBuffer)
                return;

            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.outputColor);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depth);
            if (!colorTexture || !depthTexture)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList)
                return;

            nvrhi::ITexture* normalTexture = fg.GetPhysicalTexture(data.outputNormal);
            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorTexture);
            if (normalTexture)
                fbDesc.addColorAttachment(normalTexture);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            fbDesc.setDepthAttachment(depthTexture);

            nvrhi::FramebufferHandle framebuffer = data.device->GetNVRHIDevice()->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            auto* renderDevice = data.device;
            auto& cache = framegraph::GetPassResourceCache();

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), renderDevice);
            auto detailGlobalsCB = cache.GetOrCreateVolatileCB("Detail", "DetailGlobals", sizeof(FGDetailManager::DetailFrameConstants), renderDevice);

            FGDetailManager::DetailFrameConstants frameConstants;
            dm->FillFrameConstants(frameConstants);
            cmdList->writeBuffer(detailGlobalsCB, &frameConstants, sizeof(frameConstants));

            nvrhi::IBindingSet* bindlessTable = nullptr;
            auto* backend = data.device->GetBackend();
            if (backend)
                bindlessTable = backend->GetBindlessDescriptorTable();

            auto* nvDev = data.device->GetNVRHIDevice();
            auto* shaderLoader = GEnv.Render->GetShaderLoader();

            const bool billboardMode = !ps_r__detail_gpu;

            if (billboardMode && dm->billboardGraphicsPipeline && dm->visibleBillboardInstancesBuffer &&
                dm->billboardDrawArgsBuffer && dm->pulledIndexBuffer && dm->maxPulledIndexCount > 0)
            {
                auto* bbVsRefl = shaderLoader->GetCachedReflection("detail_billboard", ".vs");
                auto* bbPsRefl = shaderLoader->GetCachedReflection("detail_billboard", ".ps");
                framegraph::BindingSetBuilder bsb(*bbVsRefl, *bbPsRefl, nvDev, "Detail.Billboard");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.ConstantBuffer("DetailGlobals", detailGlobalsCB);
                bsb.BufferSRV("visible_indices", dm->visibleBillboardInstancesBuffer);
                bsb.BufferSRV("detail_models", dm->detailModelsBuffer);
                bsb.BufferSRV("pulled_vertices", dm->pulledVertexBuffer);
                bsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                bsb.BufferSRV("slot_data", dm->slotDataBuffer);
                bsb.Texture("g_Perlin4D", dm->perlin4dTexture);
                nvrhi::BindingSetHandle bbBindingSet = cache.GetOrCreateBindingSet(bsb.Build(), dm->billboardBindingLayout, nvDev);

                nvrhi::GraphicsState state;
                state.framebuffer = framebuffer;
                state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
                state.pipeline = dm->billboardGraphicsPipeline;
                state.bindings = { bbBindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.indexBuffer = { dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                state.indirectParams = dm->billboardDrawArgsBuffer;

                cmdList->setGraphicsState(state);
                cmdList->drawIndexedIndirect(0);
            }

            if (dm->decalGraphicsPipeline && dm->visibleDecalInstancesBuffer && dm->decalDrawArgsBuffer && dm->pulledIndexBuffer && dm->maxPulledIndexCount > 0)
            {
                auto* decalVsRefl = shaderLoader->GetCachedReflection("detail_decal", ".vs");
                auto* decalPsRefl = shaderLoader->GetCachedReflection("detail_decal", ".ps");
                framegraph::BindingSetBuilder decalBsb(*decalVsRefl, *decalPsRefl, nvDev, "Detail.Decal");
                decalBsb.ConstantBuffer("DetailGlobals", detailGlobalsCB);
                decalBsb.BufferSRV("visible_indices", dm->visibleDecalInstancesBuffer);
                decalBsb.BufferSRV("detail_models", dm->detailModelsBuffer);
                decalBsb.BufferSRV("decal_vertices", dm->pulledVertexBuffer);
                decalBsb.BufferSRV("all_instances", dm->generatedInstancesBuffer);
                decalBsb.BufferSRV("slot_data", dm->slotDataBuffer);
                nvrhi::BindingSetHandle decalBindingSet = cache.GetOrCreateBindingSet(decalBsb.Build(), dm->decalBindingLayout, nvDev);

                nvrhi::GraphicsState state;
                state.framebuffer = framebuffer;
                state.viewport.addViewportAndScissorRect(nvrhi::Viewport((float)data.width, (float)data.height));
                state.pipeline = dm->decalGraphicsPipeline;
                state.bindings = { decalBindingSet };
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
                state.indexBuffer = { dm->pulledIndexBuffer, nvrhi::Format::R16_UINT, 0 };
                state.indirectParams = dm->decalDrawArgsBuffer;

                cmdList->setGraphicsState(state);
                cmdList->drawIndexedIndirect(0);
            }
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.normal = passData.outputNormal;
    outputs.baseColor = passData.baseColor;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::fg::passes
