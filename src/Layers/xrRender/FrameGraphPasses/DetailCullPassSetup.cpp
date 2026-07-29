#include "stdafx.h"
#include "DetailCullPassSetup.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"

#include <algorithm>

extern ENGINE_API float ps_r3_grass_blade_width;

namespace xray::render::fg
{
// Defined in xrRender_console.cpp (same namespace)
extern int ps_r__detail_radius;
extern float dm_current_fade;
}

namespace xray::render::fg::passes
{
using namespace framegraph;
using xray::render::fg::ps_r__detail_radius;
using xray::render::fg::dm_current_fade;

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
    Fmatrix capturedPrevViewProj;
    bool hasPrevViewProj = (prevViewProj != nullptr);
    if (hasPrevViewProj)
        capturedPrevViewProj = *prevViewProj;
    else
        capturedPrevViewProj.identity();

    fg.addCallbackPass<DetailCullPassData>(
        "DetailCull",
        [&, hiZPyramid, hiZWidth, hiZHeight, hiZMipLevels, capturedPrevViewProj, hasPrevViewProj, gpuProfiler, detailState](
            FrameGraph& builder, PassHandle passHandle, DetailCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            // Run on graphics queue so CascadedShadows (grass cast) sees fresh instances.
            // Async compute has no FG dependency into the shadow pass.
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

            nvrhi::ITexture* hiZTexture = fg.GetPhysicalTexture(data.hiZPyramid);
            // World fade from r__detail_radius (UI slider). Recompute each frame so
            // options that write the cvar without CCC still apply. Was far_plane → no-op.
            const u32 size = u32(iFloor(float(ps_r__detail_radius) / 4.f) * 2);
            float fadeDistance = float(2 * size) - 0.5f;
            if (fadeDistance < 1.f)
                fadeDistance = float(ps_r__detail_radius);
            if (g_pGamePersistent)
            {
                const float farPlane = g_pGamePersistent->Environment().CurrentEnv.far_plane;
                if (farPlane > 1.f)
                    fadeDistance = std::min(fadeDistance, farPlane);
            }
            // Keep classic cache vars in sync for any other readers
            dm_current_fade = fadeDistance;
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
