#include "stdafx.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/FMesh.hpp"
#include "Common/LevelStructure.hpp"
#include "xrEngine/x_ray.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrCore/stream_reader.h"

#if !defined(USE_DX9) && !defined(USE_OGL)
#include "Layers/xrRender/FHierrarhyVisual.h"
#include "Layers/xrRenderDX10/3DFluid/dx103DFluidVolume.h"
#endif

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

void CRender::level_Load(IReader* fs)
{
    R_ASSERT(g_pGameLevel);
    R_ASSERT(!b_loaded);

    // Begin
    pApp->LoadBegin();
    Resources->DeferredLoad(TRUE);
    IReader* chunk;

    // Shaders
    g_pGamePersistent->SetLoadStageTitle("st_loading_shaders");
    g_pGamePersistent->LoadTitle();
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
        g_pGamePersistent->SetLoadStageTitle("st_loading_geometry");
        g_pGamePersistent->LoadTitle();
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
        g_pGamePersistent->SetLoadStageTitle("st_loading_spatial_db");
        g_pGamePersistent->LoadTitle();
        chunk = fs->open_chunk(fsL_VISUALS);
        LoadVisuals(chunk);
        chunk->close();

        // Details
        g_pGamePersistent->SetLoadStageTitle("st_loading_details");
        g_pGamePersistent->LoadTitle();
        Details->Load();
    }

    // Sectors
    g_pGamePersistent->SetLoadStageTitle("st_loading_sectors_portals");
    g_pGamePersistent->LoadTitle();
    LoadSectors(fs);

#if !defined(USE_DX9) && !defined(USE_OGL)
    // 3D Fluid
    Load3DFluid();
#endif

    // HOM
    HOM.Load();

    // Lights
    g_pGamePersistent->SetLoadStageTitle("st_loading_lights");
    g_pGamePersistent->LoadTitle();
    LoadLights(fs);

    // End
    pApp->LoadEnd();

    // sanity-clear
    lstLODgroups.clear();
    mapLOD.clear();

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
    pLastSector = nullptr;
    vLastCameraPos.set(0, 0, 0);

    // 2.
    for (IRender_Sector* sector : Sectors)
        xr_delete(sector);
    Sectors.clear();

    // 3.
    for (IRender_Portal* portal : Portals)
        xr_delete(portal);
    Portals.clear();

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
            Msg("* [Loading VB] %d verts, %d Kb", vCount, (vCount * vSize) / 1024);

            // Create and fill
            //  TODO: DX10: Check fragmentation.
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
            Msg("* [Loading IB] %d indices, %d Kb", iCount, (iCount * 2) / 1024);

            // Create and fill
            //  TODO: DX10: Check fragmentation.
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
    R_ASSERT(0 == size % sizeof(CPortal::b_portal));

    const u32 count = size / sizeof(CPortal::b_portal);
    Portals.resize(count);

    for (u32 c = 0; c < count; c++)
        Portals[c] = xr_new<CPortal>();

    // load sectors
    IReader* S = fs->open_chunk(fsL_SECTORS);
    for (u32 i = 0;; i++)
    {
        IReader* P = S->open_chunk(i);
        if (!P)
            break;

        CSector* __S = xr_new<CSector>();
        __S->load(*P);
        Sectors.push_back(__S);

        P->close();
    }
    S->close();

    // load portals
    if (count)
    {
        bool do_rebuild = true;
        const bool use_cache = strstr(Core.Params, "-cdb_cache");

        string_path fName;
        strconcat(fName, "cdb_cache" DELIMITER, FS.get_path("$level$")->m_Add, "portals.bin");
        FS.update_path(fName, "$app_data_root$", fName);

        // build portal model
        rmPortals = xr_new<CDB::MODEL>();
        rmPortals->set_version(fs->get_age());
        if (use_cache && FS.exist(fName) && rmPortals->deserialize(fName))
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
        for (u32 i = 0; i < count; i++)
        {
            CPortal::b_portal P;
            fs->r(&P, sizeof(P));
            CPortal* __P = (CPortal*)Portals[i];
            __P->Setup(P.vertices.begin(), P.vertices.size(), (CSector*)getSector(P.sector_front),
                (CSector*)getSector(P.sector_back));
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

    // debug
    //	for (int d=0; d<Sectors.size(); d++)
    //		Sectors[d]->DebugDump	();

    pLastSector = nullptr;
}

void CRender::LoadSWIs(CStreamReader* base_fs)
{
    // allocate memory for portals
    if (base_fs->find_chunk(fsL_SWIS))
    {
        CStreamReader* fs = base_fs->open_chunk(fsL_SWIS);
        u32 item_count = fs->r_u32();

        xr_vector<FSlideWindowItem>::iterator it = SWIs.begin();
        xr_vector<FSlideWindowItem>::iterator it_e = SWIs.end();

        for (; it != it_e; ++it)
            xr_free((*it).sw);

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

#if !defined(USE_DX9) && !defined(USE_OGL)
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
                dx103DFluidVolume* pVolume = xr_new<dx103DFluidVolume>();
                pVolume->Load("", F, 0);

                //	Attach to sector's static geometry
                CSector* pSector = (CSector*)detectSector(pVolume->getVisData().sphere.P);
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
