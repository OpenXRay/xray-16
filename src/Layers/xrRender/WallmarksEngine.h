// WallmarksEngine.h: interface for the CWallmarksEngine class.
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "xrCore/intrusive_ptr.h"
#include "xrEngine/Render.h"

namespace WallmarksEngine
{
struct wm_slot;
}

class CSkeletonWallmark;
class CKinematics;

class CWallmarksEngine
{
public:
    typedef WallmarksEngine::wm_slot wm_slot;

public:
    struct static_wallmark
    {
        Fsphere bounds;
        xr_vector<FVF::LIT> verts;
        float ttl;
    };
    using StaticWMVec = xr_vector<static_wallmark*>;
    using WMSlotVec = xr_vector<wm_slot*>;

private:
    StaticWMVec static_pool;
    WMSlotVec marks;
    ref_geom hGeom;

    Fvector sml_normal;
    CFrustum sml_clipper;
    sPoly sml_poly_dest;
    sPoly sml_poly_src;

    xrXRC xrc;
    CDB::Collector sml_collector;
    xr_vector<u32> sml_adjacency;

    Lock lock;

private:
    wm_slot* FindSlot(const ref_shader& shader);
    wm_slot* AppendSlot(const ref_shader& shader);

private:
    void BuildMatrix(Fmatrix& dest, float invsz, const Fvector& from);
    void RecurseTri(u32 T, Fmatrix& mView, static_wallmark& W);
    void AddWallmark_internal(
        CDB::TRI* pTri, const Fvector* pVerts, const Fvector& contact_point, const ref_shader& hTexture, float sz);

    static_wallmark* static_wm_allocate();
    void static_wm_render(static_wallmark* W, FVF::LIT*& V);
    void static_wm_destroy(static_wallmark* W);

    void skeleton_wm_render(intrusive_ptr<CSkeletonWallmark>, FVF::LIT*& V);

public:
    CWallmarksEngine();
    ~CWallmarksEngine();
    // edit wallmarks
    void AddStaticWallmark(
        CDB::TRI* pTri, const Fvector* pVerts, const Fvector& contact_point, const ref_shader& hTexture, float sz);
    void AddSkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
    void AddSkeletonWallmark(
        const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);

    // render
    void Render();

    void clear();
};
