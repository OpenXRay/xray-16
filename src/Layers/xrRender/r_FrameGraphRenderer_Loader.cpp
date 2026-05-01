#include "stdafx.h"

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/FMesh.hpp"
#include "Common/LevelStructure.hpp"
#include "xrEngine/IGame_Persistent.h"
#include "xrCore/stream_reader.h"

#if defined(USE_DX11)
#include "Layers/xrRender/FHierrarhyVisual.h"
#include "Layers/xrRenderDX11/3DFluid/dx113DFluidVolume.h"
#endif

// Mega-buffer system integration
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/GPUCullingManager.h"

// D3D12: Shader compilation
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/PBRConverter/PBRTextureConverter.h"
#include "Layers/xrRender/Light_DB.h"
#include "Layers/xrRender/ModelPool.h"
#include "Layers/xrRender/WallmarksEngine.h"
#include "Layers/xrRender/r__sector.h"
#include "Layers/xrRenderDX11/DetailManager.h"

namespace xray::render
{
using namespace fg;

void FrameGraphRenderer::level_Load(IReader* fs)
{
    ZoneScoped;

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

            // D3D12: Compile NVRHI shaders directly (NO legacy ref_shader!)
            if (true) {
                CompileLevelShader(i, n_sh, firstTexture);
            }
        }
        chunk->close();
    }

    // ═══════════════════════════════════════════════════
    // D3D12: PSO PRECOMPILATION (eliminates runtime hitches!)
    // ═══════════════════════════════════════════════════
    if ((true) && !GEnv.isDedicatedServer) {
        g_pGamePersistent->LoadTitle("st_precompiling_pso");
        PrecompileLevelPSOs();
    }

    // Components
    m_pWallmarksEngine = xr_new<CWallmarksEngine>();
    m_pDetailManager = xr_new<CDetailManager>();

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
            gpuCulling->EndLevelLoad();
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

    // End
    g_pGamePersistent->LoadEnd();

    // signal loaded
    b_loaded = TRUE;
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

    // Use the CRender's ShaderLoader instance
    if (!GEnv.Render->GetShaderLoader()) {
        Msg("! [ERROR] ShaderLoader not available for shader: %s", shaderName);
        return;
    }

    // ═══════════════════════════════════════════════════
    //  COMPILE VERTEX SHADER
    // ═══════════════════════════════════════════════════
    auto vsResult = GEnv.Render->GetShaderLoader()->LoadVertexShader(shaderName, "main");

    if (vsResult.handle) {
        compiled.vsHandle = vsResult.handle;
        // Move ownership of reflection data
        compiled.vsReflection.reset(vsResult.reflection);
        vsResult.reflection = nullptr;  // Prevent double deletion
    } else {
        Msg("! [ERROR] Failed to compile VS for shader: %s", shaderName);
        return;
    }

    // ═══════════════════════════════════════════════════
    //  COMPILE PIXEL SHADER
    // ═══════════════════════════════════════════════════
    auto psResult = GEnv.Render->GetShaderLoader()->LoadPixelShader(shaderName, "main");

    if (psResult.handle) {
        compiled.psHandle = psResult.handle;
        // Move ownership of reflection data
        compiled.psReflection.reset(psResult.reflection);
        psResult.reflection = nullptr;  // Prevent double deletion
    } else {
        Msg("! [ERROR] Failed to compile PS for shader: %s", shaderName);
        return;
    }

    // ═══════════════════════════════════════════════════
    //  GET MATERIAL INFO (from MaterialSystem)
    // ═══════════════════════════════════════════════════
    compiled.materialInfo = MaterialSystem::Instance().GetMaterialInfo(shaderName);

    Msg("* Compiled shader %u: %s (VS=%p, PS=%p, alphaTest=%d, transparent=%d)",
        shaderID, shaderName,
        compiled.vsHandle.Get(),
        compiled.psHandle.Get(),
        compiled.materialInfo.alphaTest,
        compiled.materialInfo.transparent);
}

// ═══════════════════════════════════════════════════
//  D3D12: Precompile PSOs for all level shaders
// ═══════════════════════════════════════════════════
void FrameGraphRenderer::PrecompileLevelPSOs()
{
    ZoneScopedN("Precompile Level PSOs");

    auto* materialCache = GetMaterialCache();
    if (!materialCache) {
        Msg("! [ERROR] MaterialCache not available - skipping PSO precompilation");
        return;
    }

    // ═══════════════════════════════════════════════════
    //  HARDCODE FRAMEBUFFER FORMATS (matches ForwardColorPassSetup)
    // ═══════════════════════════════════════════════════
    nvrhi::Format colorFormat = nvrhi::Format::RGBA16_FLOAT;  // HDR
    nvrhi::Format depthFormat = nvrhi::Format::D32;

    u32 totalPSOs = 0;

    for (u32 shaderID = 0; shaderID < m_CompiledLevelShaders.size(); ++shaderID) {
        auto& compiled = m_CompiledLevelShaders[shaderID];

        if (!compiled.vsHandle || !compiled.psHandle)
            continue;  // Skip failed compilations

        // Update progress
        float progress = float(shaderID) / float(m_CompiledLevelShaders.size());
        g_pGamePersistent->LoadTitle("st_precompiling_pso", progress);

        // ═══════════════════════════════════════════════════
        //  FIND COMPATIBLE VERTEX FORMATS
        // ═══════════════════════════════════════════════════
        xr_vector<u32> compatibleFormats;
        for (u32 dcl_id = 0; dcl_id < BufferPool.nDC.size(); ++dcl_id) {
            if (IsVertexFormatCompatible(BufferPool.nDC[dcl_id], compiled.vsReflection.get())) {
                compatibleFormats.push_back(dcl_id);
            }
        }

        if (compatibleFormats.empty()) {
            Msg("! Shader %u (%s) has no compatible vertex formats!",
                shaderID, compiled.shaderName.c_str());
            continue;
        }

        // ═══════════════════════════════════════════════════
        //  PRECOMPILE PSOs FOR EACH FORMAT + PASS TYPE
        // ═══════════════════════════════════════════════════
        for (u32 dcl_id : compatibleFormats) {
            // 1. Forward Color PSO (always needed)
            if (CreatePrecompiledPSO(
                shaderID,
                dcl_id,
                RenderPassType::ForwardColor,
                colorFormat,
                depthFormat,
                materialCache
            )) {
                totalPSOs++;
            }

            // 2. Depth Prepass PSO (for opaque + alpha-tested)
            if (!compiled.materialInfo.transparent) {
                if (CreatePrecompiledPSO(
                    shaderID,
                    dcl_id,
                    RenderPassType::DepthPrepass,
                    nvrhi::Format::UNKNOWN,  // No color output
                    depthFormat,
                    materialCache
                )) {
                    totalPSOs++;
                }
            }
        }
    }

    Msg("* Precompiled %u PSOs for %u shaders across %u vertex formats",
        totalPSOs, m_CompiledLevelShaders.size(), BufferPool.nDC.size());
}

void FrameGraphRenderer::level_Unload()
{
    ZoneScoped;

    if (!g_pGameLevel)
        return;
    if (!b_loaded)
        return;

    // HOM
    m_HOM.Unload();

    //*** Details
    m_pDetailManager->Unload();

    //*** Sectors
    // 1.
    m_immContext.unload();
    xr_delete(m_pRmPortals);
    m_last_sector_id = IRender_Sector::INVALID_SECTOR_ID;
    Device.vCameraPositionSaved.set(0, 0, 0);

    //*** Lights
    // Glows.Unload			();
    Lights.Unload();

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

    //*** Components
    xr_delete(m_pDetailManager);
    xr_delete(m_pWallmarksEngine);

    //*** Shaders
    m_CompiledLevelShaders.clear();  // D3D12: Clear compiled NVRHI shaders
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
        visual->Load(nullptr, chunk, 0);
        BufferPool.Visuals.push_back(visual);

        chunk->close();
        index++;
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
                m_largest_sector_id = static_cast<IRender_Sector::sector_id_t>(i);
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

        m_pRmPortals = xr_new<CDB::MODEL>();
        if (use_cache)
            m_pRmPortals->set_model_crc32(crc32(fs->pointer(), chunk_size));

        string_path file_name;
        strconcat(file_name, "cdb_cache" DELIMITER, FS.get_path("$level$")->m_Add, "portals.bin");
        FS.update_path(file_name, "$app_data_root$", file_name);

        if (use_cache && FS.exist(file_name) && m_pRmPortals->deserialize(file_name, skip_crc32_check))
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
            m_pRmPortals->build(CL.getV(), CL.getVS(), CL.getT(), CL.getTS());
            if (use_cache)
                m_pRmPortals->serialize(file_name);
        }
    }
    else
    {
        m_pRmPortals = nullptr;
    }

    m_immContext.load(sectors_data, portals_data);
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

bool FrameGraphRenderer::IsFormatCompatible(u8 d3dFormat, nvrhi::Format nvrhiFormat)
{
    // TODO: Implement proper D3D format → NVRHI format conversion check
    // For now, assume compatible (vertex layout matching will catch real issues)
    return true;
}

bool FrameGraphRenderer::MatchesSemanticName(const VertexElement& elem, const xr_string& semanticName)
{
    // Map D3D11_DECL_USAGE to HLSL semantic names
    static const char* semanticNames[] = {
        "POSITION",     // D3DDECLUSAGE_POSITION = 0
        "BLENDWEIGHT",  // D3DDECLUSAGE_BLENDWEIGHT = 1
        "BLENDINDICES", // D3DDECLUSAGE_BLENDINDICES = 2
        "NORMAL",       // D3DDECLUSAGE_NORMAL = 3
        "PSIZE",        // D3DDECLUSAGE_PSIZE = 4
        "TEXCOORD",     // D3DDECLUSAGE_TEXCOORD = 5
        "TANGENT",      // D3DDECLUSAGE_TANGENT = 6
        "BINORMAL",     // D3DDECLUSAGE_BINORMAL = 7
        "TESSFACTOR",   // D3DDECLUSAGE_TESSFACTOR = 8
        "POSITIONT",    // D3DDECLUSAGE_POSITIONT = 9
        "COLOR",        // D3DDECLUSAGE_COLOR = 10
        "FOG",          // D3DDECLUSAGE_FOG = 11
        "DEPTH",        // D3DDECLUSAGE_DEPTH = 12
        "SAMPLE",       // D3DDECLUSAGE_SAMPLE = 13
    };

    if (elem.Usage >= sizeof(semanticNames) / sizeof(semanticNames[0]))
        return false;

    const char* declSemantic = semanticNames[elem.Usage];

    // Match semantic name + index (e.g., "TEXCOORD0")
    char expected[64];
    xr_sprintf(expected, "%s%u", declSemantic, elem.UsageIndex);

    return semanticName == expected || semanticName == declSemantic;
}

bool FrameGraphRenderer::IsVertexFormatCompatible(const VertexDeclarator& decl, const framegraph::ExtractedReflection* vsReflection)
{
    if (!vsReflection)
        return false;

    // Check if vertex declaration provides all inputs required by shader
    for (const auto& input : vsReflection->vertexInputSignature.elements) {
        bool found = false;

        for (u32 i = 0; decl[i].Stream != 0xFF; ++i) {
            if (MatchesSemanticName(decl[i], input.semanticName.c_str())) {
                // Check format compatibility
                if (IsFormatCompatible(decl[i].Type, input.format)) {
                    found = true;
                    break;
                }
            }
        }

        // If required input not found, formats are incompatible
        if (!found) {
            return false;
        }
    }

    return true;
}

void FrameGraphRenderer::SetupDepthState(RenderPassType passType, const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc)
{
    using RenderPassType = RenderPassType;

    switch (passType) {
    case RenderPassType::DepthPrepass:
        // Depth prepass: write depth, test with Less
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = true;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::ForwardColor:
        // Forward color: read depth (early-Z), no write, test with Equal
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = false;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::HUD:
        // HUD: test and write with LessEqual (renders in front)
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = true;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    case RenderPassType::UI:
        // UI: depth disabled
        psoDesc.renderState.depthStencilState.depthTestEnable = false;
        psoDesc.renderState.depthStencilState.depthWriteEnable = false;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;

    default:
        // Default: standard depth test
        psoDesc.renderState.depthStencilState.depthTestEnable = true;
        psoDesc.renderState.depthStencilState.depthWriteEnable = true;
        psoDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
        psoDesc.renderState.depthStencilState.stencilEnable = false;
        break;
    }
}

void FrameGraphRenderer::SetupBlendState(const MaterialSystem::MaterialInfo& materialInfo, nvrhi::GraphicsPipelineDesc& psoDesc)
{
    if (materialInfo.transparent) {
        // Transparent: alpha blending enabled
        psoDesc.renderState.blendState.targets[0].blendEnable = true;
        psoDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
        psoDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
        psoDesc.renderState.blendState.targets[0].blendOp = nvrhi::BlendOp::Add;
        psoDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
        psoDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
        psoDesc.renderState.blendState.targets[0].blendOpAlpha = nvrhi::BlendOp::Add;
    } else {
        // Opaque: no blending
        psoDesc.renderState.blendState.targets[0].blendEnable = false;
    }

    // Alpha-to-coverage for alpha test materials (optional quality improvement)
    psoDesc.renderState.blendState.alphaToCoverageEnable = materialInfo.alphaTest;
}

bool FrameGraphRenderer::CreatePrecompiledPSO(
    u32 shaderID,
    u32 vertexFormatID,
    RenderPassType passType,
    nvrhi::Format colorFormat,
    nvrhi::Format depthFormat,
    MaterialCache* materialCache)
{
    auto& compiled = m_CompiledLevelShaders[shaderID];

    // ═══════════════════════════════════════════════════
    //  CREATE PSO DESCRIPTOR
    // ═══════════════════════════════════════════════════
    nvrhi::GraphicsPipelineDesc psoDesc;
    psoDesc.VS = compiled.vsHandle;
    psoDesc.PS = compiled.psHandle;

    // Setup vertex input layout
    const VertexDeclarator& decl = BufferPool.nDC[vertexFormatID];
    u32 vb_stride = GetVertexStride(vertexFormatID);

    xr_vector<nvrhi::VertexAttributeDesc> attributes;
    for (const auto& input : compiled.vsReflection->vertexInputSignature.elements) {
        // Find matching element in vertex declaration
        for (u32 i = 0; decl[i].Stream != 0xFF; ++i) {
            if (MatchesSemanticName(decl[i], input.semanticName.c_str())) {
                nvrhi::VertexAttributeDesc attr;
                attr.name = input.semanticName.c_str();
                attr.format = input.format;  // Use format from reflection
                attr.offset = decl[i].Offset;
                attr.bufferIndex = 0;
                attr.elementStride = vb_stride;
                attr.isInstanced = false;
                attributes.push_back(attr);
                break;
            }
        }
    }

    auto* nvrhiDevice = GetRenderDevice()->GetNVRHIDevice();
    if (!nvrhiDevice) {
        Msg("! NVRHI device not available");
        return false;
    }

    if (!attributes.empty()) {
        psoDesc.inputLayout = nvrhiDevice->createInputLayout(
            attributes.data(),
            (uint32_t)attributes.size(),
            compiled.vsHandle);
    }

    // Setup depth/blend states
    SetupDepthState(passType, compiled.materialInfo, psoDesc);
    SetupBlendState(compiled.materialInfo, psoDesc);

    // Set topology (always triangles for level geometry)
    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // ═══════════════════════════════════════════════════
    //  CREATE FRAMEBUFFER INFO (required for PSO creation)
    // ═══════════════════════════════════════════════════
    nvrhi::FramebufferInfoEx fbInfo;
    if (passType == RenderPassType::ForwardColor) {
        fbInfo.addColorFormat(colorFormat);
        fbInfo.setDepthFormat(depthFormat);
    } else if (passType == RenderPassType::DepthPrepass) {
        // Depth-only: no color output
        fbInfo.setDepthFormat(depthFormat);
    }

    // ═══════════════════════════════════════════════════
    //  CREATE PSO
    // ═══════════════════════════════════════════════════
    nvrhi::GraphicsPipelineHandle pso = nvrhiDevice->createGraphicsPipeline(psoDesc, fbInfo);

    if (!pso) {
        Msg("! Failed to create PSO for shader %u (%s), format %u, pass %u",
            shaderID, compiled.shaderName.c_str(), vertexFormatID, (u32)passType);
        return false;
    }

    // Store NVRHI handle in precompiled PSO cache (MaterialCache will wrap it in MaterialPSO later)
    u64 cacheKey = ((u64)vertexFormatID << 32) | (u64)passType;

    xray::render::CompiledLevelShader::PrecompiledPSOs::PSOVariant variant;
    variant.vertexFormatID = vertexFormatID;
    variant.passType = passType;
    variant.pso = nullptr;  // fg::PipelineState not used here - we store nvrhi handle directly
    variant.materialPSO = nullptr;  // MaterialPSO creation deferred until first use

    compiled.precompiledPSOs.variants.push_back(variant);
    // Store the NVRHI handle as a MaterialPSO (temporary - will be wrapped properly on first use)
    compiled.precompiledPSOs.psoCache[cacheKey] = reinterpret_cast<MaterialPSO*>(pso.Get());

    Msg("* Precompiled PSO for shader %u (%s), format %u, pass %u",
        shaderID, compiled.shaderName.c_str(), vertexFormatID, (u32)passType);

    return true;
}

} // namespace xray::render
