#include "stdafx.h"

#include "xr_area.h"
#include "xrEngine/xr_object.h"
#include "Common/LevelStructure.hpp"
#include "xrEngine/xr_collide_form.h"


//----------------------------------------------------------------------
// Class	: CObjectSpaceData
// Purpose	: stores thread sensitive data
//----------------------------------------------------------------------
thread_local xrXRC CObjectSpaceData::xrc("object space");
thread_local collide::rq_results CObjectSpaceData::r_temp;
thread_local xr_vector<ISpatial*> CObjectSpaceData::r_spatial;

using namespace collide;

//----------------------------------------------------------------------
// Class	: CObjectSpace
// Purpose	: stores space slots
//----------------------------------------------------------------------
CObjectSpace::CObjectSpace(ISpatial_DB* spatialSpace)
    : SpatialSpace(spatialSpace)
{
#ifdef DEBUG
    if (GEnv.RenderFactory)
        m_pRender = xr_new<FactoryPtr<IObjectSpaceRender>>();
#endif
    m_BoundingVolume.invalidate();
}
//----------------------------------------------------------------------
CObjectSpace::~CObjectSpace()
{
#ifdef DEBUG
    xr_delete(m_pRender);
#endif
}
//----------------------------------------------------------------------

//----------------------------------------------------------------------
int CObjectSpace::GetNearest(xr_vector<ISpatial*>& q_spatial, xr_vector<IGameObject*>& q_nearest, const Fvector& point,
    float range, IGameObject* ignore_object)
{
    ZoneScoped;

    q_spatial.clear();
    // Query objects
    q_nearest.clear();
    Fsphere Q;
    Q.set(point, range);
    Fvector B;
    B.set(range, range, range);
    SpatialSpace->q_box(q_spatial, 0, STYPE_COLLIDEABLE, point, B);

    // Iterate
    for (auto& it : q_spatial)
    {
        IGameObject* O = it->dcast_GameObject();
        if (0 == O)
            continue;
        if (O == ignore_object)
            continue;
        Fsphere mS = {O->GetSpatialData().sphere.P, O->GetSpatialData().sphere.R};
        if (Q.intersect(mS))
            q_nearest.push_back(O);
    }

    return q_nearest.size();
}

//----------------------------------------------------------------------
int CObjectSpace::GetNearest(
    xr_vector<IGameObject*>& q_nearest, const Fvector& point, float range, IGameObject* ignore_object)
{
    return (GetNearest(r_spatial, q_nearest, point, range, ignore_object));
}

//----------------------------------------------------------------------
int CObjectSpace::GetNearest(xr_vector<IGameObject*>& q_nearest, ICollisionForm* obj, float range)
{
    IGameObject* O = obj->Owner();
    return GetNearest(q_nearest, O->GetSpatialData().sphere.P, range + O->GetSpatialData().sphere.R, O);
}

//----------------------------------------------------------------------

void CObjectSpace::Load(CDB::build_callback build_callback,
    CDB::serialize_callback serialize_callback,
    CDB::deserialize_callback deserialize_callback,
    CDB::remapping_materials_callback remapping_materials_callback)
{
    Load("$level$", "level.cform", build_callback, serialize_callback, deserialize_callback, remapping_materials_callback);
}

void CObjectSpace::Load(LPCSTR path, LPCSTR fname,
    CDB::build_callback build_callback,
    CDB::serialize_callback serialize_callback,
    CDB::deserialize_callback deserialize_callback,
    CDB::remapping_materials_callback remapping_materials_callback)
{
    IReader* F = FS.r_open(path, fname);
    R_ASSERT(F);
    Load(F, build_callback, serialize_callback, deserialize_callback, remapping_materials_callback);
}

void CObjectSpace::Load(IReader* F,
    CDB::build_callback build_callback,
    CDB::serialize_callback serialize_callback,
    CDB::deserialize_callback deserialize_callback,
    CDB::remapping_materials_callback remapping_materials_callback)
{
    ZoneScoped;

    static const bool use_cache = !strstr(Core.Params, "-no_cdb_cache");
    if (use_cache)
        Static.set_model_crc32(crc32(F->pointer(), F->length()));

    hdrCFORM H;
    F->r(&H, sizeof(hdrCFORM));

    Fvector* verts = (Fvector*)F->pointer();
    CDB::TRI* tris = (CDB::TRI*)(verts + H.vertcount);

    // SkyLoader: Check for the new format
    IReader* cacheStream = nullptr;
    size_t totalGeomSize = (static_cast<size_t>(H.vertcount) * sizeof(Fvector)) + (H.facecount * sizeof(CDB::TRI));
    F->advance(totalGeomSize);
    if (F->elapsed() > sizeof(u32))
    {
        u32 version = F->r_u32();
        if (version == CFORM_CACHE_CURRENT_VERSION)
            cacheStream = F;
    }

    Create(verts, tris, H, build_callback, serialize_callback, deserialize_callback, remapping_materials_callback, cacheStream);
    FS.r_close(F);
}

void CObjectSpace::Create(Fvector* verts, CDB::TRI* tris, const hdrCFORM& H,
    CDB::build_callback build_callback,
    CDB::serialize_callback serialize_callback,
    CDB::deserialize_callback deserialize_callback,
    CDB::remapping_materials_callback remapping_materials_callback,
    IReader* cacheStream /*= nullptr*/)
{
    ZoneScoped;

    R_ASSERT(CFORM_CURRENT_VERSION == H.version);

    string_path file_name;
    static const bool use_cache = !strstr(Core.Params, "-no_cdb_cache");
    static const bool skip_crc32_check = strstr(Core.Params, "-skip_cdb_cache_crc32_check");

    strconcat(file_name, "cdb_cache" DELIMITER, FS.get_path("$level$")->m_Add, "objspace.bin");
    FS.update_path(file_name, "$app_data_root$", file_name);

    if (use_cache && cacheStream)
    {
#ifndef MASTER_GOLD
        Msg("* Loading ObjectSpace cache from level.cform...");
#endif

        // Load geometry
        Static.load_geom(verts, H.vertcount, tris, H.facecount);

        // Read game material list
        xr_map<u16, shared_str> gameMtls;
        u32 cnt = cacheStream->r_u32();
        for (u32 i = 0; i < cnt; i++)
        {
            u16 idx = cacheStream->r_u16();
            shared_str mtlName;
            cacheStream->r_stringZ(mtlName);
            gameMtls[idx] = mtlName;
        }

        if (remapping_materials_callback)
            remapping_materials_callback(Static.get_tris(), Static.get_tris_count(), gameMtls);

        // Load OPCODE tree
        Static.deserialize_tree(cacheStream);
    }
    else if (use_cache && FS.exist(file_name) && Static.deserialize(file_name, skip_crc32_check, deserialize_callback))
    {
#ifndef MASTER_GOLD
        Msg("* Loaded ObjectSpace cache (%s)...", file_name);
#endif
    }
    else
    {
#ifndef MASTER_GOLD
        Msg("* ObjectSpace cache for '%s' was not loaded. "
            "Building the model from scratch..", file_name);
#endif
        Static.build(verts, H.vertcount, tris, H.facecount, build_callback);

        if (use_cache)
            Static.serialize(file_name, serialize_callback);
    }

    m_BoundingVolume.set(H.aabb);
}

//----------------------------------------------------------------------
#ifdef DEBUG
void CObjectSpace::dbgRender() { (*m_pRender)->dbgRender(); }
/*
void CObjectSpace::dbgRender()
{
    R_ASSERT(bDebug);

    RCache.set_Shader(sh_debug);
    for (u32 i=0; i<q_debug.boxes.size(); i++)
    {
        Fobb&		obb		= q_debug.boxes[i];
        Fmatrix		X,S,R;
        obb.xform_get(X);
        RCache.dbg_DrawOBB(X,obb.m_halfsize,color_xrgb(255,0,0));
        S.scale		(obb.m_halfsize);
        R.mul		(X,S);
        RCache.dbg_DrawEllipse(R,color_xrgb(0,0,255));
    }
    q_debug.boxes.clear();

    for (i=0; i<dbg_S.size(); i++)
    {
        std::pair<Fsphere,u32>& P = dbg_S[i];
        Fsphere&	S = P.first;
        Fmatrix		M;
        M.scale		(S.R,S.R,S.R);
        M.translate_over(S.P);
        RCache.dbg_DrawEllipse(M,P.second);
    }
    dbg_S.clear();
}
*/
#endif
// XXX stats: add to statistics
void CObjectSpace::DumpStatistics(IGameFont& font, IPerformanceAlert* alert) { xrc.DumpStatistics(font, alert); }
