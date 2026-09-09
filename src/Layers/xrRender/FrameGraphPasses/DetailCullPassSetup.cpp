#include "stdafx.h"
#include "DetailCullPassSetup.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

struct DetailCullPassData {
    VirtualResourceHandle hiZPyramid;
    VirtualResourceHandle args;
    fg::RenderDevice* device;
    fg::FGDetailManager* detailManager;
    nvrhi::ITexture* fallbackHiZ;
    u32 hiZWidth;
    u32 hiZHeight;
    u32 hiZMipLevels;
    Fmatrix prevViewProj;
    xray::profiler::GPUProfiler* gpuProfiler;
    DetailPassState* detailState;
};

VirtualResourceHandle setupDetailCullPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    VirtualResourceHandle prevHiZ,
    nvrhi::ITexture* fallbackHiZ,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    const Fmatrix& prevViewProj,
    xray::profiler::GPUProfiler* gpuProfiler,
    DetailPassState* detailState)
{
    if (!detailManager || !detailManager->drawArgsBuffer[0])
        return VirtualResourceHandle();

    ResourceDesc argsDesc;
    argsDesc.type = ResourceDesc::Type::Buffer;
    argsDesc.debugName = "DetailDrawArgsLOD0";
    argsDesc.bufferSize = sizeof(u32) * 5;
    argsDesc.isUAV = true;
    argsDesc.isTransient = false;
    VirtualResourceHandle argsHandle = fg.ImportBuffer("detail_args", detailManager->drawArgsBuffer[0], argsDesc);

    auto& passData = fg.addCallbackPass<DetailCullPassData>(
        "DetailCull",
        [&, prevHiZ, argsHandle, fallbackHiZ, hiZWidth, hiZHeight, hiZMipLevels, prevViewProj, gpuProfiler, detailState](
            FrameGraph& builder, PassHandle passHandle, DetailCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.detailManager = detailManager;
            data.fallbackHiZ = fallbackHiZ;
            data.hiZWidth = hiZWidth;
            data.hiZHeight = hiZHeight;
            data.hiZMipLevels = hiZMipLevels;
            data.prevViewProj = prevViewProj;
            data.gpuProfiler = gpuProfiler;
            data.detailState = detailState;

            if (prevHiZ.is_valid())
                data.hiZPyramid = passBuilder.read(prevHiZ, ResourceState::ShaderResource);
            data.args = passBuilder.write(argsHandle, ResourceState::UnorderedAccess);
        },
        [](const DetailCullPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("DetailCullPass", 14);

            auto* dm = data.detailManager;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!dm || !cmdList)
                return;
            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            const bool enabled = psDeviceFlags.is(rsDrawDetails) && dm->instanceGenPipeline && dm->slotDataBuffer;
            if (!enabled)
            {
                dm->ClearDrawArgs(cmdList);
                return;
            }

            if (data.detailState && !data.detailState->detailDataUploaded)
            {
                dm->UploadBufferData(cmdList);
                data.detailState->detailDataUploaded = true;
            }

            dm->UploadGrassTints(cmdList);

            if (dm->perlin4dPipeline)
            {
                if (data.gpuProfiler) data.gpuProfiler->BeginPass(cmdList, "DetailCull.Perlin");
                dm->DispatchPerlin4DCompute(cmdList, nvDevice, Device.fTimeGlobal);
                if (data.gpuProfiler) data.gpuProfiler->EndPass(cmdList, "DetailCull.Perlin");
            }

            if (dm->interactionPipeline)
            {
                if (data.gpuProfiler) data.gpuProfiler->BeginPass(cmdList, "DetailCull.Interaction");
                dm->DispatchInteraction(cmdList, nvDevice);
                if (data.gpuProfiler) data.gpuProfiler->EndPass(cmdList, "DetailCull.Interaction");
            }

            nvrhi::ITexture* hiZTexture = data.hiZPyramid.is_valid() ? fg.GetPhysicalTexture(data.hiZPyramid) : nullptr;
            u32 hiZWidth = data.hiZWidth;
            u32 hiZHeight = data.hiZHeight;
            u32 hiZMipLevels = data.hiZMipLevels;
            if (!hiZTexture)
            {
                hiZTexture = data.fallbackHiZ;
                hiZWidth = 1;
                hiZHeight = 1;
                hiZMipLevels = 0;
            }

            const float fadeDistance = g_pGamePersistent->Environment().CurrentEnv.far_plane;
            dm->DispatchCulling(
                cmdList,
                nvDevice,
                hiZTexture,
                data.prevViewProj,
                fadeDistance,
                hiZWidth,
                hiZHeight,
                hiZMipLevels,
                data.gpuProfiler
            );

            dm->ScheduleStatsReadback(cmdList, nvDevice);
        }
    );

    return passData.args;
}

} // namespace xray::render::fg::passes
