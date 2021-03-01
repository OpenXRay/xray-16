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
CObjectSpace::CObjectSpace()
#ifdef DEBUG
      : m_pRender(nullptr)
#endif
{
#ifdef DEBUG
    if (GEnv.RenderFactory)
        m_pRender = xr_new<FactoryPtr<IObjectSpaceRender>>();

// sh_debug.create				("debug\\wireframe","$null");
#endif
    m_BoundingVolume.invalidate();
}
//----------------------------------------------------------------------
CObjectSpace::~CObjectSpace()
{
// moved to ~IGameLevel
//	GEnv.Sound->set_geometry_occ		(NULL);
//	GEnv.Sound->set_handler			(NULL);
//
#ifdef DEBUG
    // sh_debug.destroy			();
    xr_delete(m_pRender);
#endif
}
//----------------------------------------------------------------------

//----------------------------------------------------------------------
int CObjectSpace::GetNearest(xr_vector<ISpatial*>& q_spatial, xr_vector<IGameObject*>& q_nearest, const Fvector& point,
    float range, IGameObject* ignore_object)
{
    q_spatial.clear();
    // Query objects
    q_nearest.clear();
    Fsphere Q;
    Q.set(point, range);
    Fvector B;
    B.set(range, range, range);
    g_SpatialSpace->q_box(q_spatial, 0, STYPE_COLLIDEABLE, point, B);

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

void CObjectSpace::Load(CDB::build_callback build_callback) { Load("$level$", "level.cform", build_callback); }
void CObjectSpace::Load(LPCSTR path, LPCSTR fname, CDB::build_callback build_callback)
{
    IReader* F = FS.r_open(path, fname);
    R_ASSERT(F);
    Load(F, build_callback);
}
void CObjectSpace::Load(IReader* F, CDB::build_callback build_callback)

{
    hdrCFORM H;
    F->r(&H, sizeof(hdrCFORM));
    Fvector* verts = (Fvector*)F->pointer();
    CDB::TRI* tris = (CDB::TRI*)(verts + H.vertcount);
    Static.set_version(F->get_age());
    Create(verts, tris, H, build_callback);
    FS.r_close(F);
}

void CObjectSpace::Create(Fvector* verts, CDB::TRI* tris, const hdrCFORM& H, CDB::build_callback build_callback)
{
    R_ASSERT(CFORM_CURRENT_VERSION == H.version);

    string_path fName;
    bool bUseCache = strstr(Core.Params, "-cdb_cache");
    strconcat(fName, "cdb_cache" DELIMITER, FS.get_path("$level$")->m_Add, "objspace.bin");
    FS.update_path(fName, "$app_data_root$", fName);
    if (bUseCache && FS.exist(fName) && Static.deserialize(fName))
    {
#ifndef MASTER_GOLD
        Msg("* Loaded ObjectSpace cache (%s)...", fName);
#endif
    }
    else
    {
#ifndef MASTER_GOLD
        Msg("* ObjectSpace cache for '%s' was not loaded. "
            "Building the model from scratch..", fName);
#endif
        Static.build(verts, H.vertcount, tris, H.facecount, build_callback);

        if (bUseCache)
            Static.serialize(fName);
    }

    m_BoundingVolume.set(H.aabb);
    g_SpatialSpace->initialize(m_BoundingVolume);
    g_SpatialSpacePhysic->initialize(m_BoundingVolume);
    // GEnv.Sound->set_geometry_occ				( &Static );
    // GEnv.Sound->set_handler					( _sound_event );
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
