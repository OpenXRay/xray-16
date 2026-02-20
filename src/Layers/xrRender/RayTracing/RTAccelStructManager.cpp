#include "stdafx.h"
#include "RTAccelStructManager.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include <nvrhi/utils.h>

namespace xray::render::RENDER_NAMESPACE {

void RTAccelStructManager::Initialize(ng::RenderDevice* device)
{
    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    m_rtSupported = nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct) &&
                    nvDevice->queryFeatureSupport(nvrhi::Feature::RayQuery);

    if (m_rtSupported)
        Msg("* [RT] Ray tracing supported (AccelStruct + RayQuery)");
    else
        Msg("* [RT] Ray tracing NOT supported on this device");
}

void RTAccelStructManager::Shutdown()
{
    m_blas = nullptr;
    m_tlas = nullptr;
    m_batchInfoBuffer = nullptr;
    m_isReady = false;
    m_batchCount = 0;
}

void RTAccelStructManager::BuildIfNeeded(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    if (m_isReady || !m_rtSupported || !gpuCulling)
        return;

    if (!gpuCulling->IsMegaDataUploaded())
        return;

    const auto& drawArgs = gpuCulling->GetStaticDrawArgsData();
    if (drawArgs.empty())
        return;

    Msg("* [RT] Building acceleration structures (%u static batches)...", (u32)drawArgs.size());

    m_megaVB = gpuCulling->GetMegaVertexBuffer();
    m_megaIB = gpuCulling->GetMegaIndexBuffer();

    BuildBLAS(cmdList, gpuCulling);
    if (!m_blas) return;

    BuildTLAS(cmdList);
    if (!m_tlas) return;

    CreateBatchInfoBuffer(cmdList, gpuCulling);

    m_isReady = true;
    Msg("* [RT] Acceleration structures ready (%u geometries: %u static, %u terrain, %u transparent)",
        m_batchCount, m_staticBatchCount, m_terrainBatchCount, m_transparentBatchCount);
}

static u32 AddBatchesToBLAS(
    nvrhi::rt::AccelStructDesc& blasDesc,
    const xr_vector<IndirectDrawArgs>& drawArgs,
    nvrhi::IBuffer* megaVB, nvrhi::IBuffer* megaIB,
    u32 totalVertexCount, u32 vertexStride,
    const xr_vector<u32>* vertexCounts = nullptr)
{
    u32 count = 0;
    for (u32 i = 0; i < (u32)drawArgs.size(); i++) {
        const auto& args = drawArgs[i];
        if (args.indexCountPerInstance == 0)
            continue;

        u32 baseVert = static_cast<u32>(args.baseVertexLocation);
        u32 batchVertexCount = (vertexCounts && i < vertexCounts->size()) ? (*vertexCounts)[i] : (totalVertexCount - baseVert);

        nvrhi::rt::GeometryTriangles tri;
        tri.setIndexBuffer(megaIB)
           .setIndexFormat(nvrhi::Format::R32_UINT)
           .setIndexOffset(static_cast<uint64_t>(args.startIndexLocation) * sizeof(u32))
           .setIndexCount(args.indexCountPerInstance)
           .setVertexBuffer(megaVB)
           .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
           .setVertexStride(vertexStride)
           .setVertexOffset(static_cast<uint64_t>(baseVert) * vertexStride)
           .setVertexCount(batchVertexCount);

        nvrhi::rt::GeometryDesc geom;
        geom.setTriangles(tri)
            .setFlags(nvrhi::rt::GeometryFlags::Opaque);

        blasDesc.addBottomLevelGeometry(geom);
        count++;
    }
    return count;
}

void RTAccelStructManager::BuildBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    constexpr u32 vertexStride = sizeof(bindless::UnifiedVertex);
    const u32 totalVerts = gpuCulling->GetTotalVertexCount();
    const auto& staticVertCounts = gpuCulling->GetStaticBatchVertexCounts();

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "SceneBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace |
                          nvrhi::rt::AccelStructBuildFlags::AllowCompaction;

    m_staticBatchCount = AddBatchesToBLAS(blasDesc, gpuCulling->GetStaticDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride, &staticVertCounts);
    m_terrainBatchCount = AddBatchesToBLAS(blasDesc, gpuCulling->GetTerrainDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride);
    m_transparentBatchCount = AddBatchesToBLAS(blasDesc, gpuCulling->GetTransparentDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride);

    m_batchCount = m_staticBatchCount + m_terrainBatchCount + m_transparentBatchCount;

    if (m_batchCount == 0) {
        Msg("! [RT] No valid geometries for BLAS");
        return;
    }

    m_blas = nvDevice->createAccelStruct(blasDesc);
    if (!m_blas) {
        Msg("! [RT] Failed to create BLAS");
        return;
    }

    nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, m_blas, blasDesc);
    cmdList->compactBottomLevelAccelStructs();
}

void RTAccelStructManager::BuildTLAS(nvrhi::ICommandList* cmdList)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    nvrhi::rt::AccelStructDesc tlasDesc;
    tlasDesc.debugName = "SceneTLAS";
    tlasDesc.isTopLevel = true;
    tlasDesc.topLevelMaxInstances = 1;
    tlasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;

    m_tlas = nvDevice->createAccelStruct(tlasDesc);
    if (!m_tlas) {
        Msg("! [RT] Failed to create TLAS");
        return;
    }

    nvrhi::rt::InstanceDesc instance;
    instance.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(0)
            .setInstanceMask(0xFF)
            .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque)
            .setBLAS(m_blas);

    cmdList->buildTopLevelAccelStruct(m_tlas, &instance, 1);
}

static void AppendBatchInfos(
    xr_vector<RTBatchInfo>& out,
    const xr_vector<IndirectDrawArgs>& drawArgs,
    const xr_vector<u32>& materialIDs)
{
    for (u32 i = 0; i < (u32)drawArgs.size(); i++) {
        if (drawArgs[i].indexCountPerInstance == 0)
            continue;
        RTBatchInfo info;
        info.materialID = (i < materialIDs.size()) ? materialIDs[i] : 0;
        info.startIndex = drawArgs[i].startIndexLocation;
        info.baseVertex = drawArgs[i].baseVertexLocation;
        info.indexCount = drawArgs[i].indexCountPerInstance;
        out.push_back(info);
    }
}

void RTAccelStructManager::CreateBatchInfoBuffer(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    xr_vector<RTBatchInfo> batchInfos;
    batchInfos.reserve(m_batchCount);

    AppendBatchInfos(batchInfos, gpuCulling->GetStaticDrawArgsData(), gpuCulling->GetStaticMaterialIDData());
    AppendBatchInfos(batchInfos, gpuCulling->GetTerrainDrawArgsData(), gpuCulling->GetTerrainMaterialIDData());
    AppendBatchInfos(batchInfos, gpuCulling->GetTransparentDrawArgsData(), gpuCulling->GetTransparentMaterialIDData());

    nvrhi::BufferDesc desc;
    desc.debugName = "RTBatchInfoBuffer";
    desc.byteSize = batchInfos.size() * sizeof(RTBatchInfo);
    desc.structStride = sizeof(RTBatchInfo);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    m_batchInfoBuffer = nvDevice->createBuffer(desc);
    cmdList->writeBuffer(m_batchInfoBuffer, batchInfos.data(), batchInfos.size() * sizeof(RTBatchInfo));
}

}
