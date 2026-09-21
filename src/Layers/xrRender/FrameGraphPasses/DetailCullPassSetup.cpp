#include "stdafx.h"
#include "DetailCullPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Profiler/GPUProfiler.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

struct DetailCullPassData
{
    VirtualResourceHandle hiZPyramid;
    std::shared_ptr<FGDetailManager::VisibilityFrame> frame;
    std::shared_ptr<FGDetailManager::VisibilityFrame> rayFrame;
    std::shared_ptr<FGDetailManager::GenerationWork> generation;
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

bool DetailPassResources::HasSource() const
{
    return frame && frame->source && !frame->source->chunks.empty();
}

void DetailPassResources::Read(RenderPassBuilder& builder, bool drawIndirect, bool swIndirect) const
{
    if (!HasSource())
        return;
    for (auto handle : sourceChunks)
        builder.read(handle, ResourceState::ShaderResource);
    for (auto handle : visible)
        builder.read(handle, ResourceState::ShaderResource);
    for (auto handle : prepared)
        builder.read(handle, ResourceState::ShaderResource);
    for (auto handle : { models, pulled, packets, perlin, interaction[0], interaction[1], tints })
        if (handle.is_valid())
            builder.read(handle, ResourceState::ShaderResource);
    if (drawIndirect || swIndirect)
        for (auto handle : drawArgs)
            builder.read(handle, drawIndirect ? ResourceState::IndirectArgument : ResourceState::ShaderResource);
    if (drawIndirect)
        for (auto handle : indices)
            builder.read(handle, ResourceState::IndexBuffer);
    if (swIndirect)
        builder.read(swDispatch, ResourceState::IndirectArgument);
}

DetailPassResources setupDetailCullPass(
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
    DetailPassResources result;
    if (!detailManager || !detailManager->visibilityFrame)
        return result;
    result.frame = detailManager->visibilityFrame;
    const auto& frame = *result.frame;
    const auto importBuffer = [&](nvrhi::IBuffer* buffer)
    {
        if (!buffer)
            return VirtualResourceHandle();
        const auto& native = buffer->getDesc();
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Buffer;
        desc.bufferSize = native.byteSize;
        desc.structStride = native.structStride;
        desc.isUAV = native.canHaveUAVs;
        desc.isIndirectArgs = native.isDrawIndirectArgs;
        desc.isTransient = false;
        desc.debugName = native.debugName.c_str();
        return fg.ImportBuffer(native.debugName.c_str(), buffer, desc);
    };
    const auto importTexture = [&](nvrhi::ITexture* texture)
    {
        if (!texture)
            return VirtualResourceHandle();
        const auto& native = texture->getDesc();
        ResourceDesc desc;
        desc.type = native.dimension == nvrhi::TextureDimension::Texture3D ? ResourceDesc::Type::Texture3D : ResourceDesc::Type::Texture2D;
        desc.width = native.width;
        desc.height = native.height;
        desc.depth = native.depth;
        desc.mipLevels = native.mipLevels;
        desc.arraySize = native.arraySize;
        desc.format = native.format;
        desc.isUAV = native.isUAV;
        desc.isTransient = false;
        desc.debugName = native.debugName.c_str();
        return fg.ImportTexture(native.debugName.c_str(), texture, desc);
    };
    for (u32 kind = 0; kind < FGDetailManager::VIS_KIND_COUNT; ++kind)
    {
        result.visible[kind] = importBuffer(frame.visible[kind]);
        result.drawArgs[kind] = importBuffer(frame.drawArgs[kind]);
    }
    for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; ++lod)
    {
        result.prepared[lod] = importBuffer(frame.prepared[lod]);
        result.indices[lod] = importBuffer(detailManager->bladeIndexBuffer[lod]);
    }
    if (result.HasSource())
    {
        result.sourceChunks.reserve(frame.visibleChunks.size());
        for (u32 chunk : frame.visibleChunks)
            result.sourceChunks.push_back(importBuffer(frame.source->chunks[chunk].buffer));
        result.models = importBuffer(frame.source->models);
        result.pulled = importBuffer(frame.source->pulledVertices);
    }
    result.packets = importBuffer(frame.packets);
    result.swDispatch = importBuffer(frame.swDispatch);
    result.tints = importBuffer(detailManager->cachedGrassTintsBuffer);
    result.perlin = importTexture(detailManager->perlin4dTexture);
    for (u32 i = 0; i < 2; ++i)
        result.interaction[i] = importTexture(detailManager->interactionTexture[i]);

    fg.addCallbackPass<DetailCullPassData>(
        "DetailCull",
        [&](FrameGraph& builder, PassHandle passHandle, DetailCullPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();
            passBuilder.sideEffects();
            data.device = device;
            data.detailManager = detailManager;
            data.frame = result.frame;
            data.generation = detailManager->generationWork;
            data.fallbackHiZ = fallbackHiZ;
            data.hiZWidth = hiZWidth;
            data.hiZHeight = hiZHeight;
            data.hiZMipLevels = hiZMipLevels;
            data.prevViewProj = prevViewProj;
            data.gpuProfiler = gpuProfiler;
            data.detailState = detailState;
            if (prevHiZ.is_valid())
                data.hiZPyramid = passBuilder.read(prevHiZ, ResourceState::ShaderResource);
            for (auto handle : result.visible)
                passBuilder.write(handle, ResourceState::UnorderedAccess);
            for (auto handle : result.drawArgs)
                passBuilder.write(handle, ResourceState::UnorderedAccess);
            for (auto handle : result.prepared)
                passBuilder.write(handle, ResourceState::UnorderedAccess);
            for (auto handle : { result.packets, result.swDispatch, importBuffer(frame.visibleSlots),
                importBuffer(frame.visibleSlotCount), importBuffer(frame.slotDispatch), importBuffer(frame.workStatus) })
                passBuilder.write(handle, ResourceState::UnorderedAccess);
            passBuilder.write(importBuffer(frame.readback), ResourceState::CopyDest);
            for (auto handle : result.sourceChunks)
                passBuilder.read(handle, ResourceState::ShaderResource);
            if (result.HasSource())
            {
                passBuilder.read(importBuffer(frame.source->slots), ResourceState::ShaderResource);
                passBuilder.read(result.models, ResourceState::ShaderResource);
                passBuilder.read(result.pulled, ResourceState::ShaderResource);
            }
            if (!detailManager->bladeIndicesUploaded)
                for (auto handle : result.indices)
                    passBuilder.write(handle, ResourceState::CopyDest);
            const bool upload = detailState && !detailState->detailDataUploaded;
            for (auto buffer : { detailManager->slotDataBuffer, detailManager->detailModelsBuffer, detailManager->pulledVertexBuffer })
            {
                const auto handle = importBuffer(buffer);
                if (upload)
                    passBuilder.write(handle, ResourceState::CopyDest);
                else
                    passBuilder.read(handle, ResourceState::ShaderResource);
            }
            if (result.tints.is_valid())
                passBuilder.write(result.tints, ResourceState::CopyDest);
            if (result.perlin.is_valid())
                passBuilder.readWrite(result.perlin, ResourceState::UnorderedAccess);
            for (auto handle : result.interaction)
                if (handle.is_valid())
                    passBuilder.readWrite(handle, ResourceState::UnorderedAccess);
            if (data.generation)
            {
                const auto& work = *data.generation;
                if (work.stage == FGDetailManager::GenerationWork::Stage::CountReady || work.stage == FGDetailManager::GenerationWork::Stage::EmitReady)
                    passBuilder.read(importTexture(detailManager->heightmapTexture), ResourceState::ShaderResource);
                if (work.stage == FGDetailManager::GenerationWork::Stage::CountReady)
                {
                    passBuilder.write(importBuffer(work.counts), ResourceState::UnorderedAccess);
                    passBuilder.write(importBuffer(work.countReadback), ResourceState::CopyDest);
                }
                else if (work.stage == FGDetailManager::GenerationWork::Stage::EmitReady)
                {
                    for (auto buffer : { work.source->slots, work.emitSlots })
                    {
                        const auto handle = importBuffer(buffer);
                        if (work.nextChunk == 0)
                            passBuilder.write(handle, ResourceState::CopyDest);
                        else
                            passBuilder.read(handle, ResourceState::ShaderResource);
                    }
                    passBuilder.readWrite(importBuffer(work.localCounters), ResourceState::UnorderedAccess);
                    passBuilder.readWrite(importBuffer(work.status), ResourceState::UnorderedAccess);
                    passBuilder.write(importBuffer(work.statusReadback), ResourceState::CopyDest);
                    const u32 end = std::min(u32(work.source->chunks.size()), work.nextChunk + fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS);
                    for (u32 i = work.nextChunk; i < end; ++i)
                        passBuilder.write(importBuffer(work.source->chunks[i].buffer), ResourceState::UnorderedAccess);
                }
            }
            const auto ray = detailManager->rayVisibilityFrame;
            if (ray && ray->source && !ray->source->chunks.empty())
            {
                data.rayFrame = ray;
                for (auto buffer : ray->visible)
                    passBuilder.write(importBuffer(buffer), ResourceState::UnorderedAccess);
                for (auto buffer : ray->drawArgs)
                    passBuilder.write(importBuffer(buffer), ResourceState::UnorderedAccess);
                for (auto buffer : ray->prepared)
                    passBuilder.write(importBuffer(buffer), ResourceState::UnorderedAccess);
                for (auto buffer : { ray->visibleSlots, ray->visibleSlotCount, ray->slotDispatch, ray->packets, ray->swDispatch, ray->workStatus })
                    passBuilder.write(importBuffer(buffer), ResourceState::UnorderedAccess);
                passBuilder.write(importBuffer(ray->readback), ResourceState::CopyDest);
                for (u32 chunk : ray->visibleChunks)
                    passBuilder.read(importBuffer(ray->source->chunks[chunk].buffer), ResourceState::ShaderResource);
                for (auto buffer : { ray->source->slots, ray->source->models, ray->source->pulledVertices })
                    passBuilder.read(importBuffer(buffer), ResourceState::ShaderResource);
            }
        },
        [](const DetailCullPassData& data, const FrameGraph& fg, fg::RenderContext* ctx)
        {
            ZoneScoped;
            ZoneName("DetailCullPass", 14);
            auto* dm = data.detailManager;
            auto* cmdList = ctx->GetCommandList();
            R_ASSERT2(cmdList && data.frame, "[DetailManager] detail culling command list or frame unavailable");
            auto* nvDevice = data.device->GetNVRHIDevice();
            if (data.detailState && !data.detailState->detailDataUploaded)
            {
                dm->UploadBufferData(cmdList);
                data.detailState->detailDataUploaded = true;
            }
            dm->UploadGrassTints(cmdList);
            if (dm->perlin4dPipeline)
            {
                if (data.gpuProfiler)
                    data.gpuProfiler->BeginPass(cmdList, "DetailCull.Perlin");
                dm->DispatchPerlin4DCompute(cmdList, nvDevice, Device.fTimeGlobal);
                if (data.gpuProfiler)
                    data.gpuProfiler->EndPass(cmdList, "DetailCull.Perlin");
            }
            if (dm->interactionPipeline)
            {
                if (data.gpuProfiler)
                    data.gpuProfiler->BeginPass(cmdList, "DetailCull.Interaction");
                dm->DispatchInteraction(cmdList, nvDevice);
                if (data.gpuProfiler)
                    data.gpuProfiler->EndPass(cmdList, "DetailCull.Interaction");
            }
            auto* hiZTexture = data.hiZPyramid.is_valid() ? fg.GetPhysicalTexture(data.hiZPyramid) : nullptr;
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
            dm->DispatchCulling(cmdList, nvDevice, hiZTexture, *data.frame, data.generation,
                data.prevViewProj, fadeDistance, hiZWidth, hiZHeight, hiZMipLevels, data.gpuProfiler);
            dm->ScheduleStatsReadback(cmdList, nvDevice, *data.frame);
            if (data.rayFrame)
            {
                dm->DispatchCulling(cmdList, nvDevice, hiZTexture, *data.rayFrame, nullptr,
                    data.prevViewProj, fadeDistance, 0, 1, 0, data.gpuProfiler);
                dm->ScheduleStatsReadback(cmdList, nvDevice, *data.rayFrame);
            }
        });
    return result;
}

}
