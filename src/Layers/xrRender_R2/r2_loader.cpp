#include "stdafx.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/FMesh.hpp"
#include "Common/LevelStructure.hpp"
#include "xrEngine/IGame_Persistent.h"
#include "xrCore/stream_reader.h"

#if defined(USE_DX11)
#include "Layers/xrRender/FHierrarhyVisual.h"
#include "Layers/xrRenderDX11/3DFluid/dx113DFluidVolume.h"
#endif

void CRender::level_Load(IReader* fs)
{
    R_ASSERT(g_pGameLevel);
    R_ASSERT(!b_loaded);

    // Begin
    g_pGamePersistent->LoadBegin();
    Resources->DeferredLoad(TRUE);
    IReader* chunk;

    // Shaders
    g_pGamePersistent->LoadTitle("st_loading_shaders");
    {
        chunk = fs->open_chunk(fsL_SHADERS);
        R_ASSERT2(chunk, "Level doesn't builded correctly.");
        u32 count = chunk->r_u32();
        Shaders.resize(count);
        for (u32 i = 0; i < count; i++) // skip first shader as "reserved" one
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
            Shaders[i] = Resources->Create(n_sh, n_tlist);
        }
        chunk->close();
    }

    // Components
    Wallmarks = xr_new<CWallmarksEngine>();
    Details = xr_new<CDetailManager>();

    if (!GEnv.isDedicatedServer)
    {
        // VB,IB,SWI
        g_pGamePersistent->LoadTitle("st_loading_geometry");
        {
            CStreamReader* geom = FS.rs_open("$level$", "level.geom");
            R_ASSERT2(geom, "level.geom");
            LoadBuffers(geom, false);
            LoadSWIs(geom);
            FS.r_close(geom);
        }

        //...and alternate/fast geometry
        {
            CStreamReader* geom = FS.rs_open("$level$", "level.geomx");
            R_ASSERT2(geom, "level.geomX");
            LoadBuffers(geom, true);
            FS.r_close(geom);
        }

        // Visuals
        g_pGamePersistent->LoadTitle("st_loading_spatial_db");
        chunk = fs->open_chunk(fsL_VISUALS);
        LoadVisuals(chunk);
        chunk->close();

        // Details
        g_pGamePersistent->LoadTitle("st_loading_details");
        Details->Load();
    }

    // Sectors
    g_pGamePersistent->LoadTitle("st_loading_sectors_portals");
    LoadSectors(fs);

#if defined(USE_DX11)
    // 3D Fluid
    Load3DFluid();
#endif

    // HOM
    HOM.Load();

    // Lights
    g_pGamePersistent->LoadTitle("st_loading_lights");
    LoadLights(fs);

    // End
    g_pGamePersistent->LoadEnd();

    // signal loaded
    b_loaded = TRUE;
}

void CRender::level_Unload()
{
    if (!g_pGameLevel)
        return;
    if (!b_loaded)
        return;

    // HOM
    HOM.Unload();

    //*** Details
    Details->Unload();

    //*** Sectors
    // 1.
    xr_delete(rmPortals);
    last_sector_id = IRender_Sector::INVALID_SECTOR_ID;
    Device.vCameraPositionSaved.set(0, 0, 0);

    // 2.
    cleanup_contexts();

    //*** Lights
    // Glows.Unload			();
    Lights.Unload();

    //*** Visuals
    for (dxRender_Visual* visual : Visuals)
    {
        visual->Release();
        xr_delete(visual);
    }
    Visuals.clear();

    //*** SWI
    for (auto& swi : SWIs)
        xr_free(swi.sw);
    SWIs.clear();

    //*** VB/IB
    for (auto& indexBuffer : nVB)
    {
        indexBuffer.Release();
    }
    nVB.clear();

    for (auto& vertexBuffer : xVB)
    {
        vertexBuffer.Release();
    }
    xVB.clear();

    for (auto& indexBuffer : nIB)
    {
        indexBuffer.Release();
    }
    nIB.clear();

    for (auto& vertexBuffer : xIB)
    {
        vertexBuffer.Release();
    }
    xIB.clear();

    nDC.clear();
    xDC.clear();

    //*** Components
    xr_delete(Details);
    xr_delete(Wallmarks);

    //*** Shaders
    Shaders.clear();
    b_loaded = FALSE;
    /*
        Models->ClearPool( true );
        Visuals.clear();
        dxRenderDeviceRender::Instance().Resources->Dump(false);
        static int unload_counter = 0;
        Msg("The Level Unloaded.======================== %d", ++unload_counter);
    */
}

void CRender::LoadBuffers(CStreamReader* base_fs, bool alternative)
{
    R_ASSERT2(base_fs, "Could not load geometry. File not found.");
    Resources->Evict();
    // Vertex buffers
    {
        xr_vector<VertexDeclarator>& _DC = alternative ? xDC : nDC;
        xr_vector<VertexStagingBuffer>& _VB = alternative ? xVB : nVB;

        // Use DX9-style declarators
        CStreamReader* fs = base_fs->open_chunk(fsL_VB);
        R_ASSERT2(fs, "Could not load geometry. File 'level.geom?' corrupted.");

        const u32 count = fs->r_u32();
        _DC.resize(count);
        _VB.resize(count);

        constexpr size_t buffer_size = (MAXD3DDECLLENGTH + 1) * sizeof(VertexElement);
        for (u32 i = 0; i < count; i++)
        {
            // decl
            VertexElement* dcl = (VertexElement*)xr_alloca(buffer_size);
            fs->r(dcl, buffer_size);
            fs->advance(-(int)buffer_size);

            const u32 dcl_len = GetDeclLength(dcl) + 1;
            _DC[i].resize(dcl_len);
            fs->r(_DC[i].begin(), dcl_len * sizeof(VertexElement));

            // count, size
            const u32 vCount = fs->r_u32();
            const u32 vSize = GetDeclVertexSize(dcl, 0);
#ifndef MASTER_GOLD
            Msg("* [Loading VB] %d verts, %d Kb", vCount, (vCount * vSize) / 1024);
#endif

            // Create and fill
            //  TODO: DX11: Check fragmentation.
            //  Check if buffer is less then 2048 kb
            _VB[i].Create(vCount * vSize);
            u8* pData = static_cast<u8*>(_VB[i].Map());
            fs->r(pData, vCount * vSize);
            _VB[i].Unmap(true); // upload vertex data

            //			fs->advance			(vCount*vSize);
        }
        fs->close();
    }

    // Index buffers
    {
        xr_vector<IndexStagingBuffer>& _IB = alternative ? xIB : nIB;

        CStreamReader* fs = base_fs->open_chunk(fsL_IB);
        const u32 count = fs->r_u32();
        _IB.resize(count);
        for (u32 i = 0; i < count; i++)
        {
            const u32 iCount = fs->r_u32();
#ifndef MASTER_GOLD
            Msg("* [Loading IB] %d indices, %d Kb", iCount, (iCount * 2) / 1024);
#endif

            // Create and fill
            //  TODO: DX11: Check fragmentation.
            //  Check if buffer is less then 2048 kb
            _IB[i].Create(iCount * 2);
            u8* pData = static_cast<u8*>(_IB[i].Map());
            fs->r(pData, iCount * 2);
            _IB[i].Unmap(true); // upload index data

            //			fs().advance		(iCount*2);
        }
        fs->close();
    }
}

void CRender::LoadVisuals(IReader* fs)
{
    u32 index = 0;
    IReader* chunk = nullptr;

    while ((chunk = fs->open_chunk(index)) != 0)
    {
        ogf_header H;
        chunk->r_chunk_safe(OGF_HEADER, &H, sizeof(H));

        dxRender_Visual* visual = Models->Instance_Create(H.type);
        visual->Load(nullptr, chunk, 0);
        Visuals.push_back(visual);

        chunk->close();
        index++;
    }
}

void CRender::LoadLights(IReader* fs)
{
    // lights
    Lights.Load(fs);
    Lights.LoadHemi();
}

void CRender::LoadSectors(IReader* fs)
{
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
                largest_sector_id = static_cast<IRender_Sector::sector_id_t>(i);
            }
        }
        P->close();
    }
    S->close();

    // load portals
    if (portals_count)
    {
        bool do_rebuild = true;
        const bool use_cache = !strstr(Core.Params, "-no_cdb_cache");
        const bool checkCrc32 = !strstr(Core.Params, "-skip_cdb_cache_crc32_check");

        string_path fName;
        strconcat(fName, "cdb_cache" DELIMITER, FS.get_path("$level$")->m_Add, "portals.bin");
        FS.update_path(fName, "$app_data_root$", fName);

        // build portal model
        rmPortals = xr_new<CDB::MODEL>();
        rmPortals->set_version(fs->get_age());
        if (use_cache && FS.exist(fName) && rmPortals->deserialize(fName, checkCrc32))
        {
#ifndef MASTER_GOLD
            Msg("* Loaded portals cache (%s)...", fName);
#endif
            do_rebuild = false;
        }
        else
        {
#ifndef MASTER_GOLD
            Msg("* Portals cache for '%s' was not loaded. "
                "Building the model from scratch..", fName);
#endif
        }

        CDB::Collector CL;
        fs->find_chunk(fsL_PORTALS);
        for (u32 i = 0; i < portals_count; i++)
        {
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
            rmPortals->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));
            if (use_cache)
                rmPortals->serialize(fName);
        }
    }
    else
    {
        rmPortals = nullptr;
    }

    for (int id = 0; id < R__NUM_PARALLEL_CONTEXTS; ++id)
    {
        auto& dsgraph = contexts_pool[id];
        dsgraph.reset();
        dsgraph.load(sectors_data, portals_data);
        contexts_used.set(id, false);
    }

    auto& dsgraph = get_imm_context();
    dsgraph.reset();
    dsgraph.load(sectors_data, portals_data);

    last_sector_id = IRender_Sector::INVALID_SECTOR_ID;
}

void CRender::LoadSWIs(CStreamReader* base_fs)
{
    // allocate memory for portals
    if (base_fs->find_chunk(fsL_SWIS))
    {
        CStreamReader* fs = base_fs->open_chunk(fsL_SWIS);
        u32 item_count = fs->r_u32();

        for (auto& SWI : SWIs)
            xr_free(SWI.sw);

        SWIs.clear();

        SWIs.resize(item_count);
        for (u32 c = 0; c < item_count; c++)
        {
            FSlideWindowItem& swi = SWIs[c];
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

#if defined(USE_DX11)
void CRender::Load3DFluid()
{
    // if (strstr(Core.Params,"-no_volumetric_fog"))
    if (!o.volumetricfog)
        return;

    string_path fn_game;
    if (FS.exist(fn_game, "$level$", "level.fog_vol"))
    {
        IReader* F = FS.r_open(fn_game);
        u16 version = F->r_u16();

        if (version == 3)
        {
            u32 cnt = F->r_u32();
            for (u32 i = 0; i < cnt; ++i)
            {
                dx113DFluidVolume* pVolume = xr_new<dx113DFluidVolume>();
                pVolume->Load("", F, 0);

                auto& dsgraph = get_imm_context();

                //	Attach to sector's static geometry
                const auto sector_id = dsgraph.detect_sector(pVolume->getVisData().sphere.P);
                auto* pSector = static_cast<CSector*>(dsgraph.get_sector(sector_id));
                //	3DFluid volume must be in render sector
                VERIFY(pSector);

                dxRender_Visual* pRoot = pSector->root();
                //	Sector must have root
                VERIFY(pRoot);
                VERIFY(pRoot->getType() == MT_HIERRARHY);

                ((FHierrarhyVisual*)pRoot)->children.push_back(pVolume);
            }
        }

        FS.r_close(F);
    }
}
#endif
