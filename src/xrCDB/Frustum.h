// Frustum.h: interface for the CFrustum class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "xrCDB.h"

#include "xrCore/FixedVector.h"
#include "xrCore/_plane.h"

#pragma pack(push, 4)

enum EFC_Visible
{
    fcvNone = 0,
    fcvPartial,
    fcvFully,
    fcv_forcedword = u32(-1)
};

#define FRUSTUM_MAXPLANES 12
#define FRUSTUM_P_LEFT (1 << 0)
#define FRUSTUM_P_RIGHT (1 << 1)
#define FRUSTUM_P_TOP (1 << 2)
#define FRUSTUM_P_BOTTOM (1 << 3)
#define FRUSTUM_P_NEAR (1 << 4)
#define FRUSTUM_P_FAR (1 << 5)

#define FRUSTUM_P_LRTB (FRUSTUM_P_LEFT | FRUSTUM_P_RIGHT | FRUSTUM_P_TOP | FRUSTUM_P_BOTTOM)
#define FRUSTUM_P_ALL (FRUSTUM_P_LRTB | FRUSTUM_P_NEAR | FRUSTUM_P_FAR)

#define FRUSTUM_SAFE (FRUSTUM_MAXPLANES * 4)
typedef svector<Fvector, FRUSTUM_SAFE> sPoly;
extern u32 frustum_aabb_remap[8][6];

class XRCDB_API CFrustum
{
public:
    struct fplane : public Fplane
    {
        u32 aabb_overlap_id; // [0..7]
        void cache();
    };
    fplane planes[FRUSTUM_MAXPLANES];
    size_t p_count;

public:
    ICF EFC_Visible AABB_OverlapPlane(const fplane& P, const float* mM) const
    {
        // calc extreme pts (neg,pos) along normal axis (pos in dir of norm, etc.)
        u32* id = frustum_aabb_remap[P.aabb_overlap_id];

        Fvector Neg;
        Neg.set(mM[id[3]], mM[id[4]], mM[id[5]]);
        if (P.classify(Neg) > 0)
            return fcvNone;

        Fvector Pos;
        Pos.set(mM[id[0]], mM[id[1]], mM[id[2]]);
        if (P.classify(Pos) <= 0)
            return fcvFully;

        return fcvPartial;
    }

public:
    IC void _clear() { p_count = 0; }
    void _add(Fplane& P);
    void _add(Fvector& P1, Fvector& P2, Fvector& P3);

    void SimplifyPoly_AABB(sPoly* P, Fplane& plane);

    void CreateOccluder(Fvector* p, size_t count, Fvector& vBase, CFrustum& clip);
    bool CreateFromClipPoly(
        Fvector* p, size_t count, Fvector& vBase, CFrustum& clip); // returns 'false' if creation failed
    void CreateFromPoints(Fvector* p, size_t count, Fvector& vBase);
    void CreateFromMatrix(Fmatrix& M, u32 mask);
    void CreateFromPortal(sPoly* P, Fvector& vPN, Fvector& vBase, Fmatrix& mFullXFORM);
    void CreateFromPlanes(Fplane* p, size_t count);

    sPoly* ClipPoly(sPoly& src, sPoly& dest) const;

    u32 getMask() const { return (1 << p_count) - 1; }
    EFC_Visible testSphere(Fvector& c, float r, u32& test_mask) const;
    bool testSphere_dirty(Fvector& c, float r) const;
    EFC_Visible testAABB(const float* mM, u32& test_mask) const;
    EFC_Visible testSAABB(Fvector& c, float r, const float* mM, u32& test_mask) const;
    bool testPolyInside_dirty(Fvector* p, size_t count) const;

    IC bool testPolyInside(sPoly& src) const
    {
        sPoly d;
        return !!ClipPoly(src, d);
    }
    IC bool testPolyInside(Fvector* p, size_t count) const
    {
        sPoly src(p, count);
        return testPolyInside(src);
    }
};
#pragma pack(pop)
