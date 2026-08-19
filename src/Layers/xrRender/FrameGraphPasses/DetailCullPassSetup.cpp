#include "stdafx.h"
#include "DetailCullPassSetup.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"

extern ENGINE_API float ps_r3_grass_blade_width;

namespace xray::render::fg::passes
{
using namespace framegraph;

struct DetailCullPassData {
    VirtualResourceHandle hiZPyramid;
    fg::RenderDevice* device;
    fg::FGDetailManager* detailManager;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
    Fmatrix prevViewProj;
    bool hasPrevViewProj;
    xray::profiler::GPUProfiler* gpuProfiler;
    DetailPassState* detailState;
};

void setupDetailCullPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    VirtualResourceHandle hiZPyramid,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    const Fmatrix* prevViewProj,
    xray::profiler::GPUProfiler* gpuProfiler,
    DetailPassState* detailState)
{
    if (!detailManager || !hiZPyramid.is_valid() || hiZMipLevels == 0)
        return;

    Fmatrix capturedPrevViewProj;
    bool hasPrevViewProj = (prevViewProj != nullptr);
    if (hasPrevViewProj)
        capturedPrevViewProj = *prevViewProj;
    else
        capturedPrevViewProj.identity();

    nvrhi::IBuffer* cullFence = nullptr;
    if (detailManager) {
        if (detailManager->billboardDrawArgsBuffer)
            cullFence = detailManager->billboardDrawArgsBuffer;
        else if (detailManager->drawArgsBuffer[0])
            cullFence = detailManager->drawArgsBuffer[0];
        else if (detailManager->decalDrawArgsBuffer)
            cullFence = detailManager->decalDrawArgsBuffer;
    }
    VirtualResourceHandle cullArgsHandle{};
    if (cullFence) {
        ResourceDesc fenceDesc;
        fenceDesc.type = ResourceDesc::Type::Buffer;
        fenceDesc.debugName = "DetailCull_Args";
        fenceDesc.bufferSize = sizeof(u32) * 5;
        fenceDesc.structStride = sizeof(u32);
        fenceDesc.isUAV = true;
        fenceDesc.isTransient = false;
        fenceDesc.isImported = true;
        cullArgsHandle = fg.ImportBuffer("detail_cull_args", cullFence, fenceDesc);
        if (detailState)
            detailState->cullArgs = cullArgsHandle;
    }

    fg.addCallbackPass<DetailCullPassData>(
        "DetailCull",
        [&, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels, capturedPrevViewProj, hasPrevViewProj, gpuProfiler, detailState, cullArgsHandle](
            FrameGraph& builder, PassHandle passHandle, DetailCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();
            passBuilder.sideEffects();

            data.device = device;
            data.detailManager = detailManager;
            data.hiZWidth = hiZWidth;
            data.hiZHeight = hiZHeight;
            data.hiZMipLevels = hiZMipLevels;
            data.prevViewProj = capturedPrevViewProj;
            data.hasPrevViewProj = hasPrevViewProj;
            data.gpuProfiler = gpuProfiler;
            data.detailState = detailState;

            data.hiZPyramid = passBuilder.read(hiZPyramid, ResourceState::ShaderResource);
            if (cullArgsHandle.is_valid())
                passBuilder.write(cullArgsHandle, ResourceState::UnorderedAccess);
        },
        [](const DetailCullPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("DetailCullPass", 14);

            if (!data.detailManager)
                return;

            if (!psDeviceFlags.is(rsDrawDetails))
                return;

            bool detailPipelineValid = (data.detailManager->instanceGenPipeline && data.detailManager->slotDataBuffer);
            if (!detailPipelineValid)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!cmdList)
                return;

            nvrhi::ITexture* hiZTexture = fg.GetPhysicalTexture(data.hiZPyramid);
            if (!hiZTexture)
                return;

            if (data.detailState && !data.detailState->detailDataUploaded)
            {
                data.detailManager->UploadBufferData(cmdList);
                data.detailState->detailDataUploaded = true;
            }

            if (data.detailState && data.detailState->lastBladeWidth != ps_r3_grass_blade_width)
            {
                data.detailManager->RegenerateBladeGeometry(cmdList);
                data.detailState->lastBladeWidth = ps_r3_grass_blade_width;
            }

            const float fadeDistance = g_pGamePersistent->Environment().CurrentEnv.far_plane;
            Fmatrix effectivePrevViewProj = data.hasPrevViewProj ? data.prevViewProj : Device.mFullTransform;

            data.detailManager->DispatchCulling(
                cmdList,
                data.device->GetNVRHIDevice(),
                hiZTexture,
                effectivePrevViewProj,
                fadeDistance,
                data.hiZWidth,
                data.hiZHeight,
                data.hiZMipLevels,
                data.gpuProfiler
            );

            data.detailManager->ScheduleStatsReadback(cmdList, data.device->GetNVRHIDevice());
        }
    );
}

} // namespace xray::render::fg::passes
