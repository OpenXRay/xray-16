#include "stdafx.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/Threading/TaskManager.hpp"
#include "Common/LevelStructure.hpp"
#include "xrEngine/IGame_Persistent.h"
#include "xrCore/stream_reader.h"

#if defined(USE_DX11)
#include "Layers/xrRender/FHierrarhyVisual.h"
#endif

// Mega-buffer system integration
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/GpuParticleManager.h"
#include "Layers/xrRender/ParticleEffectDef.h"
#include "Layers/xrRender/ParticleEditor/ParticleEditor.h"
#include "Layers/xrRender/FrameGraph/Blackboard.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/FrameGraphPasses/VSMPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/LocalShadowPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/DistortionApplyPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/TransparentPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/DecalPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/GpuParticlePassSetup.h"

// D3D12: Shader compilation
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/PBRConverter/PBRTextureConverter.h"
#include "Layers/xrRender/Light_DB.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/r__sector.h"
#include "Layers/xrRender/r__scene.h"
#include "Layers/xrRender/FVisual.h"
#include "Layers/xrRender/FProgressive.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/RayTracing/WorldRadianceCache.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/Decals/OverlayManager.h"
#include "xrEngine/IRenderBackend.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"

namespace xray::render
{
using namespace fg;

void FrameGraphRenderer::level_Load(IReader* fs)
{
    ZoneScoped;
    m_lightingState.ResetRecovery();

    R_ASSERT(g_pGameLevel);
    R_ASSERT(!b_loaded);

    // Begin
    g_pGamePersistent->LoadBegin();
    Resources->DeferredLoad(TRUE);
    IReader* chunk;

    // ═══════════════════════════════════════════════════
    // CRITICAL: Load vertex formats BEFORE shaders (needed for PSO precompilation)
    // ═══════════════════════════════════════════════════
    if (!GEnv.isDedicatedServer)
    {
        // ═══════════════════════════════════════════════════════
        //  MEGA-BUFFER SYSTEM: Begin level load
        // ═══════════════════════════════════════════════════════
        GPUCullingManager* gpuCulling = nullptr;
        if (true) {
            gpuCulling = GetGPUCullingManager();
            if (gpuCulling) {
                // Estimate geometry size (will be refined during LoadBuffers)
                gpuCulling->BeginLevelLoad(2000000, 6000000);
            }
        }
        if (m_geometryCollector)
            m_geometryCollector->ClearStatic();
        if (m_blackboard) {
            passes::InvalidateVSMCache(m_blackboard->get_or_add<passes::VSMState>());
            passes::ResetLocalShadowPool(m_blackboard->get_or_add<passes::LocalShadowState>());
        }
        passes::ResetSunDirVisual();

        // VB,IB,SWI - MOVED UP! Must load vertex formats before compiling shaders
        g_pGamePersistent->LoadTitle("st_loading_geometry");
        {
            CStreamReader* geom = FS.rs_open("$level$", "level.geom");
            R_ASSERT2(geom, "level.geom");
            LoadBuffers(geom, false);
            LoadSWIs(geom);
            FS.r_close(geom);
        }

        //...and alternate/fast geometry
        if (CStreamReader* geom = FS.rs_open("$level$", "level.geomX"))
        {
            LoadBuffers(geom, true);
            FS.r_close(geom);
            BufferPool.fastGeomLoaded = true;
        }
    }

    // Shaders - NOW LOADS AFTER VERTEX FORMATS
    g_pGamePersistent->LoadTitle("st_loading_shaders");
    {
        ZoneScopedN("Load shaders");
        chunk = fs->open_chunk(fsL_SHADERS);
        R_ASSERT2(chunk, "Level doesn't builded correctly.");
        u32 count = chunk->r_u32();
        m_CompiledLevelShaders.resize(count);  // D3D12: Compiled NVRHI shaders
        for (u32 i = 0; i < count; i++)
        {
            string512 n_sh, n_tlist;
            LPCSTR n = LPCSTR(chunk->pointer());
            chunk->skip_stringZ();
            if (0 == n[0])
                continue;
            xr_strcpy(n_sh, n);
            pstr delim = strchr(n_sh, '/');
            *delim = 0;
            xr_strcpy(n_tlist, delim + 1);

            // Extract first texture name
            string256 firstTexture;
            xr_strcpy(firstTexture, n_tlist);
            if (pstr comma = strchr(firstTexture, ','))
                *comma = 0;  // Truncate at first comma

            if (true) {
                CompileLevelShader(i, n_sh, firstTexture);
            }
        }
        chunk->close();
    }

    if (!GEnv.isDedicatedServer)
    {
        // BufferPool.Visuals
        g_pGamePersistent->LoadTitle("st_loading_spatial_db");
        chunk = fs->open_chunk(fsL_VISUALS);
        LoadVisuals(chunk);
        chunk->close();

        // ═══════════════════════════════════════════════════════
        //  MEGA-BUFFER SYSTEM: End level load
        // ═══════════════════════════════════════════════════════
        auto* gpuCulling = GetGPUCullingManager();
        if (gpuCulling) {
            g_pGamePersistent->LoadTitle("st_loading_geometry");
            xr_vector<ClusterBakeRange> ranges;
            CollectClusterBakeRanges(ranges);

            u64 geomStamp = 0;
            {
                static u8 s_stampBuf[65536];
                auto hashSource = [&](const char* name) {
                    CStreamReader* geom = FS.rs_open("$level$", name);
                    if (!geom)
                        return;
                    const u64 total = geom->length();
                    u32 crc = 0;
                    u64 read = 0;
                    while (read < total) {
                        const u32 chunkSize = u32(std::min<u64>(total - read, sizeof(s_stampBuf)));
                        geom->r(s_stampBuf, chunkSize);
                        crc = crc32(s_stampBuf, chunkSize, crc);
                        read += chunkSize;
                    }
                    FS.r_close(geom);
                    geomStamp = geomStamp * 1099511628211ull;
                    geomStamp ^= total;
                    geomStamp = geomStamp * 1099511628211ull;
                    geomStamp ^= u64(crc);
                };
                hashSource("level.geom");
                hashSource("level.geomx");
            }

            string_path cachePath = "";
            {
                string_path levelName = "level";
                if (g_pGameLevel && g_pGameLevel->name().size())
                    xr_strcpy(levelName, g_pGameLevel->name().c_str());
                string_path rel;
                xr_sprintf(rel, "cluster_cache_fg%s%s.vclf", DELIMITER, levelName);
                FS.update_path(cachePath, "$app_data_root$", rel);
            }

            gpuCulling->BakeClusterDAG(ranges, cachePath, geomStamp);
        }
        if (gpuCulling) {
            gpuCulling->EndLevelLoad();
            if (GEnv.Backend)
                GEnv.Backend->WaitForIdle();
            xr_set<const VertexStagingBuffer*> retainedVertices;
            xr_set<const IndexStagingBuffer*> retainedIndices;
            for (auto* visual : BufferPool.Visuals)
            {
                IRender_Mesh* mesh = nullptr;
                bool skinned = false;
                switch (visual->getType())
                {
                case MT_NORMAL: mesh = static_cast<Fvisual*>(visual); break;
                case MT_PROGRESSIVE: mesh = static_cast<FProgressive*>(visual); break;
                case MT_TREE_ST:
                case MT_TREE_PM: mesh = static_cast<FTreeVisual*>(visual); break;
                case MT_SKELETON_GEOMDEF_ST:
                    mesh = static_cast<CSkeletonX_ST*>(visual);
                    skinned = true;
                    break;
                case MT_SKELETON_GEOMDEF_PM:
                    mesh = static_cast<CSkeletonX_PM*>(visual);
                    skinned = true;
                    break;
                default: break;
                }
                if (mesh && (skinned || !gpuCulling->GetMeshAllocation(mesh->vbPoolID,
                    mesh->vBase, mesh->vCount, mesh->ibPoolID, mesh->iBase, mesh->iCount,
                    mesh->useAlternativeGeom).valid))
                {
                    retainedVertices.insert(mesh->p_rm_Vertices);
                    retainedIndices.insert(mesh->p_rm_Indices);
                }
            }
            auto discard = [&](auto& buffers, const auto& retained)
            {
                for (auto& buffer : buffers)
                {
                    Resources->DiscardGeometryBuffer(buffer.GetBufferHandle());
                    buffer.DiscardDeviceBuffer();
                    if (retained.find(&buffer) == retained.end())
                        buffer.DiscardHostBuffer();
                }
            };
            discard(BufferPool.nVB, retainedVertices);
            discard(BufferPool.xVB, retainedVertices);
            discard(BufferPool.nIB, retainedIndices);
            discard(BufferPool.xIB, retainedIndices);
        }

        // Details
        g_pGamePersistent->LoadTitle("st_loading_details");

        // FGDetailManager: Load details for framegraph renderer
        if (true) {
            auto* detailMgr = GetDetailManager();
            if (detailMgr && detailMgr->Load()) {
                detailMgr->BakeHeightmap();
                detailMgr->LoadHeightmapTexture(GetRenderDevice()->GetNVRHIDevice());
                pbr::PBRConversionParams pbrParams;
                pbrParams.generate_mipmaps = true;
                pbr::ConvertSingleTextureToPBR("$level$", "build_details.dds", pbrParams);
                detailMgr->LoadBuildDetailsTexture(GetRenderDevice()->GetNVRHIDevice());
                detailMgr->ComputeSlotAABBs();
                detailMgr->CreateGPUBuffers(GetRenderDevice()->GetNVRHIDevice());

                // NOTE: Initial grass generation happens automatically on first frame
                // (m_lastDensity starts at -1, triggering regeneration in DispatchCulling)
            }
        }

        // Legacy CDetailManager (TODO: remove once FGDetailManager is fully working)
        // Details->Load();
    }

    // Sectors
    g_pGamePersistent->LoadTitle("st_loading_sectors_portals");
    LoadSectors(fs);

    // HOM - Skip if using FrameGraph renderer (GPU Hi-Z culling replaces CPU HOM)
    if (!true)
    {
        m_HOM.Load();
    }
    else
    {
        Msg("* [FrameGraph] Skipping HOM load - using GPU Hi-Z culling instead");
    }

    // Lights
    g_pGamePersistent->LoadTitle("st_loading_lights");
    LoadLights(fs);

    if (!GEnv.isDedicatedServer && m_blackboard)
        passes::WarmLocalShadowPool(GetRenderDevice(), m_blackboard->get_or_add<passes::LocalShadowState>());

    if (!GEnv.isDedicatedServer)
    {
        WarmParticles();
        WarmGameplayPipelines();
    }

    // End
    g_pGamePersistent->LoadEnd();

    if (GEnv.Backend)
        GEnv.Backend->SavePipelineCache();

    // signal loaded
    b_loaded = TRUE;
}

void FrameGraphRenderer::WarmParticles()
{
    ZoneScoped;
    g_pGamePersistent->LoadTitle("st_loading_textures");
    if (m_materialCache)
    {
        sh_list textures;
        for (auto it = m_PSLibrary.FirstPED(); it != m_PSLibrary.LastPED(); ++it)
        {
            const PS::CPEDef& def = **it;
            if (!def.m_Flags.is(PS::CPEDef::dfSprite) || !def.m_TextureName.size())
                continue;
            Resources->_ParseList(textures, def.m_TextureName.c_str());
            if (!textures.empty())
                m_materialCache->PreRegisterParticleMaterial(textures[0], strstr(def.m_ShaderName.c_str(), "distort") != nullptr);
        }
        m_materialCache->FinalizePendingMaterials();
    }
    if (nvrhi::IDevice* device = m_device ? m_device->GetNVRHIDevice() : nullptr)
    {
        GetGpuParticleManager().WarmPipelines(device);
        if (m_blackboard)
            passes::InitializeDistortionApplyPass(device, m_blackboard->get_or_add<passes::DistortionApplyPassState>());
    }
}

void FrameGraphRenderer::WarmGameplayPipelines()
{
    ZoneScoped;
    if (!m_device || !m_blackboard)
        return;

    g_pGamePersistent->LoadTitle("st_precompiling_pso");
    auto& cache = framegraph::GetPassResourceCache();
    const framegraph::PassResourceCache::Stats before = cache.GetStats();
    CTimer timer;
    timer.Start();

    passes::WarmTransparentPipelines(m_device, m_blackboard->get_or_add<passes::TransparentPassState>());
    passes::WarmDecalPipeline(m_device, m_blackboard->get_or_add<passes::DecalPassState>());
    passes::WarmGpuParticlePipelines(m_device, m_blackboard->get_or_add<passes::GpuParticlePassState>());

    const framegraph::PassResourceCache::Stats& after = cache.GetStats();
    Msg("* [PipelineWarm] %u pipelines created in %.1f ms",
        (after.pipelineMisses - before.pipelineMisses) + (after.computePipelineMisses - before.computePipelineMisses),
        timer.GetElapsed_sec() * 1000.f);
}

// ═══════════════════════════════════════════════════
//  D3D12: Compile shaders using NVRHI ShaderLoader
// ═══════════════════════════════════════════════════
void FrameGraphRenderer::CompileLevelShader(u32 shaderID, const char* shaderName, const char* textureName)
{
    ZoneScopedN("Compile Level Shader");

    auto& compiled = m_CompiledLevelShaders[shaderID];
    compiled.shaderName = shaderName;
    compiled.textureName = textureName;

    // ═══════════════════════════════════════════════════
    //  GET MATERIAL INFO (from MaterialSystem)
    // ═══════════════════════════════════════════════════
    compiled.materialInfo = MaterialSystem::Instance().GetMaterialInfo(shaderName);

    Msg("* Registered level shader %u: %s (alphaTest=%d, transparent=%d)",
        shaderID, shaderName,
        compiled.materialInfo.alphaTest,
        compiled.materialInfo.transparent);
}

// ═══════════════════════════════════════════════════
//  D3D12: Precompile PSOs for all level shaders
// ═══════════════════════════════════════════════════
void FrameGraphRenderer::level_Unload()
{
    ZoneScoped;
    m_lightingState.ResetRecovery();
    m_mainView.InvalidateHistory();
    passes::DiscardPathTracerSnapshot(m_mainView.pathTracer);
    if (m_particleEditor)
        m_particleEditor->OnLevelUnload();
    if (m_processHOMTask) {
        TaskScheduler->Wait(m_processHOMTask);
        m_processHOMTask.Reset();
    }

    if (m_geometryCollector) {
        m_geometryCollector->ClearStatic();
        m_geometryCollector->BeginFrame();
    }

    if (!g_pGameLevel)
        return;
    if (!b_loaded)
        return;
    if (GEnv.Backend)
    {
        GEnv.Backend->WaitForIdle();
        GEnv.Backend->SavePipelineCache();
    }
    if (m_framegraph)
        m_framegraph->Reset();
    framegraph::GetPassResourceCache().ClearBindingSets();
    if (m_gpuCullingManager)
        m_gpuCullingManager->UnloadLevel();
    if (m_rtAccelMgr)
    {
        m_rtAccelMgr->Shutdown();
        m_rtAccelMgr->Initialize(m_device);
    }
    if (m_worldCache)
        m_worldCache->Invalidate();
    if (m_skyEnvironment)
        m_skyEnvironment->Invalidate();
    if (m_detailManager)
        m_detailManager->Unload();
    m_hudBatches.clear();
    GetGpuParticleManager().LevelUnload();

    // HOM
    m_HOM.Unload();

    //*** Sectors
    Scene.unload();
    m_last_sector_id = IRender_Sector::INVALID_SECTOR_ID;
    Device.vCameraPositionSaved.set(0, 0, 0);

    //*** Lights
    // Glows.Unload			();
    Lights.Unload();

    if (m_blackboard) {
        passes::InvalidateVSMCache(m_blackboard->get_or_add<passes::VSMState>());
        passes::ResetLocalShadowPool(m_blackboard->get_or_add<passes::LocalShadowState>());
    }
    passes::ResetSunDirVisual();

    //*** BufferPool.Visuals
    for (dxRender_Visual* visual : BufferPool.Visuals)
    {
        visual->Release();
        xr_delete(visual);
    }
    BufferPool.Visuals.clear();

    //*** SWI
    for (auto& swi : BufferPool.SWIs)
        xr_free(swi.sw);
    BufferPool.SWIs.clear();

    //*** VB/IB
    for (auto& indexBuffer : BufferPool.nVB)
    {
        indexBuffer.Release();
    }
    BufferPool.nVB.clear();

    for (auto& vertexBuffer : BufferPool.xVB)
    {
        vertexBuffer.Release();
    }
    BufferPool.xVB.clear();

    for (auto& indexBuffer : BufferPool.nIB)
    {
        indexBuffer.Release();
    }
    BufferPool.nIB.clear();

    for (auto& vertexBuffer : BufferPool.xIB)
    {
        vertexBuffer.Release();
    }
    BufferPool.xIB.clear();

    BufferPool.nDC.clear();
    BufferPool.xDC.clear();

    BufferPool.fastGeomLoaded = false;

    //*** Shaders
    m_CompiledLevelShaders.clear();  // D3D12: Clear compiled NVRHI shaders
    CleanupDeveloperLoad();
    if (m_overlayManager)
        m_overlayManager->Clear();
    fg::ClusteredLightManager::Instance().ReleaseSpotTextures();
    if (auto* resourceManager = m_device ? m_device->GetFGResourceManager() : nullptr)
        m_postProcess.ReleaseTextures(resourceManager->GetTextureManager());
    if (m_materialCache)
        m_materialCache->ReleaseLevelMaterials();
    b_loaded = FALSE;
    if (ps_r__clear_models_on_unload)
    {
        g_pModelPool->ClearPool(true);
        BufferPool.Visuals.clear();
        Resources->Dump(false);
        //static int unload_counter = 0;
        //Msg("The Level Unloaded.======================== %d", ++unload_counter);
    }
}

void FrameGraphRenderer::LoadBuffers(CStreamReader* base_fs, bool alternative)
{
    ZoneScoped;

    R_ASSERT2(base_fs, "Could not load geometry. File not found.");
    Resources->Evict();

    // Get GPUCullingManager for mega-buffer registration
    GPUCullingManager* gpuCulling = nullptr;
    if (true) {
        gpuCulling = GetGPUCullingManager();
    }

    // Vertex buffers
    {
        ZoneScopedN("Load VBs");
        xr_vector<VertexDeclarator>& decls = alternative ? BufferPool.xDC : BufferPool.nDC;
        xr_vector<VertexStagingBuffer>& vbuffers = alternative ? BufferPool.xVB : BufferPool.nVB;

        // Use DX9-style declarators
        CStreamReader* fs = base_fs->open_chunk(fsL_VB);
        R_ASSERT2(fs, "Could not load geometry. File 'level.geom?' corrupted.");

        const u32 count = fs->r_u32();
        decls.resize(count);
        vbuffers.resize(count);

        constexpr size_t buffer_size = (XR_MAX_DECL_LENGTH + 1) * sizeof(VertexElement);
        for (u32 i = 0; i < count; i++)
        {
            // decl
            VertexElement* dcl = (VertexElement*)xr_alloca(buffer_size);
            fs->r(dcl, buffer_size);
            fs->advance(-(int)buffer_size);

            const u32 dcl_len = GetDeclLength(dcl) + 1;
            decls[i].resize(dcl_len);
            fs->r(decls[i].begin(), dcl_len * sizeof(VertexElement));

            // count, size
            const u32 vCount = fs->r_u32();
            const u32 vSize = GetDeclVertexSize(dcl, 0);
#ifndef MASTER_GOLD
            Msg("* [Loading VB] %d verts, %d Kb", vCount, (vCount * vSize) / 1024);
#endif

            // Create and fill
            //  TODO: DX11: Check fragmentation.
            //  Check if buffer is less then 2048 kb
            vbuffers[i].Create(vCount * vSize);
            u8* pData = static_cast<u8*>(vbuffers[i].Map());
            fs->r(pData, vCount * vSize);

            // ═══════════════════════════════════════════════════════
            //  MEGA-BUFFER: Register VB pool before upload
            // ═══════════════════════════════════════════════════════
            if (gpuCulling) {
                gpuCulling->RegisterVBPool(pData, vCount, vSize, dcl, alternative);
            }

            vbuffers[i].Unmap(true); // upload vertex data

            //			fs->advance			(vCount*vSize);
        }
        fs->close();
    }

    // Index buffers
    {
        ZoneScopedN("Load IBs");
        xr_vector<IndexStagingBuffer>& ibuffers = alternative ? BufferPool.xIB : BufferPool.nIB;

        CStreamReader* fs = base_fs->open_chunk(fsL_IB);
        const u32 count = fs->r_u32();
        ibuffers.resize(count);
        for (u32 i = 0; i < count; i++)
        {
            const u32 iCount = fs->r_u32();
#ifndef MASTER_GOLD
            Msg("* [Loading IB] %d indices, %d Kb", iCount, (iCount * 2) / 1024);
#endif

            // Create and fill
            //  TODO: DX11: Check fragmentation.
            //  Check if buffer is less then 2048 kb
            ibuffers[i].Create(iCount * 2);
            u8* pData = static_cast<u8*>(ibuffers[i].Map());
            fs->r(pData, iCount * 2);

            // ═══════════════════════════════════════════════════════
            //  MEGA-BUFFER: Register IB pool before upload
            // ═══════════════════════════════════════════════════════
            if (gpuCulling) {
                gpuCulling->RegisterIBPool(reinterpret_cast<const u16*>(pData), iCount, alternative);
            }

            ibuffers[i].Unmap(true); // upload index data

            //			fs().advance		(iCount*2);
        }
        fs->close();
    }
}

void FrameGraphRenderer::LoadVisuals(IReader* fs)
{
    u32 index = 0;
    IReader* chunk = nullptr;

    ZoneScoped;

    while ((chunk = fs->open_chunk(index)) != 0)
    {
        ogf_header H;
        chunk->r_chunk_safe(OGF_HEADER, &H, sizeof(H));

        dxRender_Visual* visual = g_pModelPool->Instance_Create(H.type);
        string64 name;
        xr_sprintf(name, "@level_visual:%u", index);
        visual->Load(name, chunk, 0);
        BufferPool.Visuals.push_back(visual);

        chunk->close();
        index++;
    }
}

void FrameGraphRenderer::CollectClusterBakeRanges(xr_vector<fg::ClusterBakeRange>& ranges)
{
    auto* gpuCulling = GetGPUCullingManager();
    if (!gpuCulling)
        return;

    ranges.reserve(BufferPool.Visuals.size());

    xr_map<fg::ClusterMeshKey, u32> refCounts;

    auto makeRange = [](const MeshAllocation& alloc)
    {
        fg::ClusterBakeRange range;
        range.key.vertexOffset = alloc.vertexOffset;
        range.key.indexOffset = alloc.indexOffset;
        range.key.vertexCount = alloc.vertexCount;
        range.key.indexCount = alloc.indexCount;
        range.flags = 0;
        return range;
    };

    for (dxRender_Visual* visual : BufferPool.Visuals)
    {
        if (!visual)
            continue;

        IRender_Mesh* mesh = nullptr;
        u32 iBase = 0, iCount = 0;
        bool mergeableType = false;

        switch (visual->getType())
        {
        case MT_NORMAL:
            mesh = static_cast<Fvisual*>(visual);
            iBase = mesh->iBase;
            iCount = mesh->iCount;
            mergeableType = true;
            break;
        case MT_PROGRESSIVE:
        {
            auto* pm = static_cast<FProgressive*>(visual);
            mesh = pm;
            const FSlideWindowItem& swi = pm->GetSWI();
            if (swi.sw && swi.count > 0)
            {
                iBase = pm->iBase + swi.sw[0].offset;
                iCount = swi.sw[0].num_tris * 3;
            }
            else
            {
                iBase = pm->iBase;
                iCount = pm->iCount;
            }
            mergeableType = true;
            break;
        }
        case MT_TREE_ST:
            mesh = static_cast<FTreeVisual_ST*>(visual);
            iBase = mesh->iBase;
            iCount = mesh->iCount;
            break;
        case MT_TREE_PM:
        {
            auto* tree = static_cast<FTreeVisual_PM*>(visual);
            mesh = tree;
            const FSlideWindowItem* swi = tree->GetSWI();
            if (swi && swi->sw && swi->count > 0)
            {
                iBase = tree->iBase + swi->sw[0].offset;
                iCount = swi->sw[0].num_tris * 3;
            }
            else
            {
                iBase = tree->iBase;
                iCount = tree->iCount;
            }
            break;
        }
        default:
            continue;
        }

        if (!mesh || iCount < 3)
            continue;

        const bool isTerrain = m_materialCache && m_materialCache->IsTerrainMaterial(visual);


        MeshAllocation alloc = gpuCulling->GetMeshAllocation(
            mesh->vbPoolID, mesh->vBase, mesh->vCount,
            mesh->ibPoolID, iBase, iCount, mesh->useAlternativeGeom);
        if (!alloc.valid)
            continue;
        const auto& materialInfo = MaterialSystem::Instance().GetMaterialInfo(visual->shaderName, visual->textureName);
        const bool forward = !isTerrain && materialInfo.transparent;
        if (forward)
        {
            const u32 materialID = m_materialCache
                ? m_materialCache->PreRegisterBindlessMaterial(visual) : UINT32_MAX;
            if (fg::GPUCullingManager::MaterialCastsShadow(materialID))
            {
                fg::ClusterBakeRange range = makeRange(alloc);
                range.flags = fg::CLUSTER_RANGE_FLAG_AT;
                ranges.push_back(range);
                refCounts[range.key]++;
            }
            gpuCulling->RetainForwardGeometry(alloc);
            continue;
        }
        fg::ClusterBakeRange range = makeRange(alloc);
        if (isTerrain) {
            range.flags |= fg::CLUSTER_RANGE_FLAG_TERRAIN;
            if (mergeableType)
                range.flags |= fg::CLUSTER_RANGE_FLAG_MERGEABLE;
        }
        else if (materialInfo.alphaTest)
        {
            range.flags |= fg::CLUSTER_RANGE_FLAG_AT;
        } else if (mergeableType) {
            range.flags |= fg::CLUSTER_RANGE_FLAG_MERGEABLE;
        }
        ranges.push_back(range);
        refCounts[range.key]++;
    }

    for (fg::ClusterBakeRange& range : ranges) {
        if (refCounts[range.key] > 1)
            range.flags &= ~fg::CLUSTER_RANGE_FLAG_MERGEABLE;
    }
}

void FrameGraphRenderer::LoadLights(IReader* fs)
{
    ZoneScoped;
    // lights
    Lights.Load(fs);
    Lights.LoadHemi();
}

void FrameGraphRenderer::LoadSectors(IReader* fs)
{
    ZoneScoped;

    // allocate memory for portals
    const u32 size = fs->find_chunk(fsL_PORTALS);
    R_ASSERT(0 == size % sizeof(CPortal::level_portal_data_t));

    const u32 portals_count = size / sizeof(CPortal::level_portal_data_t);
    xr_vector<CPortal::level_portal_data_t> portals_data{portals_count};

    // load sectors
    xr_vector<CSector::level_sector_data_t> sectors_data;

    float largest_sector_vol = 0.0f;
    IReader* S = fs->open_chunk(fsL_SECTORS);
    for (u32 i = 0;; i++)
    {
        IReader* P = S->open_chunk(i);
        if (!P)
            break;

        ZoneScopedN("Load sector");
        auto& sector_data = sectors_data.emplace_back();
        {
            u32 size = P->find_chunk(fsP_Portals);
            R_ASSERT(0 == (size & 1));
            u32 portals_in_sector = size / sizeof(u16);

            sector_data.portals_id.reserve(portals_in_sector);
            while (portals_in_sector)
            {
                const u16 ID = P->r_u16();
                sector_data.portals_id.emplace_back(ID);
                --portals_in_sector;
            }

            size = P->find_chunk(fsP_Root);
            R_ASSERT(size == 4);
            sector_data.root_id = P->r_u32();

            // Search for default sector - assume "default" or "outdoor" sector is the largest one
            // XXX: hack: need to know real outdoor sector
            auto* V = static_cast<dxRender_Visual*>(RImplementation.getVisual(sector_data.root_id));
            float vol = V->vis.box.getvolume();
            if (vol > largest_sector_vol)
            {
                largest_sector_vol = vol;
                Scene.largest_sector_id = static_cast<IRender_Sector::sector_id_t>(i);
            }
        }
        P->close();
    }
    S->close();

    // load portals
    if (portals_count)
    {
        static const bool use_cache = !strstr(Core.Params, "-no_cdb_cache");
        static const bool skip_crc32_check = strstr(Core.Params, "-skip_cdb_cache_crc32_check");

        ZoneScopedN("Load portals");

        // build portal model
        bool do_rebuild = true;
        const auto chunk_size = fs->find_chunk(fsL_PORTALS);

        Scene.rmPortals = xr_new<CDB::MODEL>();
        if (use_cache)
            Scene.rmPortals->set_model_crc32(crc32(fs->pointer(), chunk_size));

        string_path file_name;
        strconcat(file_name, "cdb_cache" DELIMITER, FS.get_path("$level$")->m_Add, "portals.bin");
        FS.update_path(file_name, "$app_data_root$", file_name);

        if (use_cache && FS.exist(file_name) && Scene.rmPortals->deserialize(file_name, skip_crc32_check))
        {
#ifndef MASTER_GOLD
            Msg("* Loaded portals cache (%s)...", file_name);
#endif
            do_rebuild = false;
        }
        else
        {
#ifndef MASTER_GOLD
            Msg("* Portals cache for '%s' was not loaded. "
                "Building the model from scratch..", file_name);
#endif
        }

        CDB::Collector CL;
        for (u32 i = 0; i < portals_count; i++)
        {
            ZoneScopedN("Build portal from chunk");
            auto &P = portals_data[i];
            fs->r(&P, sizeof(P));

            if (do_rebuild)
            {
                for (u32 j = 2; j < P.vertices.size(); j++)
                    CL.add_face_packed_D(P.vertices[0], P.vertices[j - 1], P.vertices[j], u32(i));
            }
        }

        if (do_rebuild)
        {
            if (CL.getTS() < 2)
            {
                Fvector v1, v2, v3;
                v1.set(-20000.f, -20000.f, -20000.f);
                v2.set(-20001.f, -20001.f, -20001.f);
                v3.set(-20002.f, -20002.f, -20002.f);
                CL.add_face_packed_D(v1, v2, v3, 0);
            }
            Scene.rmPortals->build(CL.getV(), CL.getVS(), CL.getT(), CL.getTS());
            if (use_cache)
                Scene.rmPortals->serialize(file_name);
        }
    }
    else
    {
        Scene.rmPortals = nullptr;
    }

    Scene.load(sectors_data, portals_data);
    m_last_sector_id = IRender_Sector::INVALID_SECTOR_ID;
}

void FrameGraphRenderer::LoadSWIs(CStreamReader* base_fs)
{
    ZoneScoped;

    // allocate memory for portals
    if (base_fs->find_chunk(fsL_SWIS))
    {
        CStreamReader* fs = base_fs->open_chunk(fsL_SWIS);
        u32 item_count = fs->r_u32();

        for (auto& SWI : BufferPool.SWIs)
            xr_free(SWI.sw);

        BufferPool.SWIs.clear();

        BufferPool.SWIs.resize(item_count);
        for (u32 c = 0; c < item_count; c++)
        {
            FSlideWindowItem& swi = BufferPool.SWIs[c];
            swi.reserved[0] = fs->r_u32();
            swi.reserved[1] = fs->r_u32();
            swi.reserved[2] = fs->r_u32();
            swi.reserved[3] = fs->r_u32();
            swi.count = fs->r_u32();
            VERIFY(nullptr == swi.sw);
            swi.sw = xr_alloc<FSlideWindow>(swi.count);
            fs->r(swi.sw, sizeof(FSlideWindow) * swi.count);
        }
        fs->close();
    }
}

u32 FrameGraphRenderer::GetVertexStride(u32 vertexFormatID)
{
    if (vertexFormatID >= BufferPool.nDC.size())
        return 0;
    const VertexDeclarator& decl = BufferPool.nDC[vertexFormatID];
    return GetDeclVertexSize(decl.begin(), 0);  // Stream 0
}

} // namespace xray::render
