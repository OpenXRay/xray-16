// Portal.h: interface for the CPortal class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/_fbox2.h"
#include "xrCDB/Frustum.h"

namespace CDB { class MODEL; }
class xrXRC;

namespace xray::render::fg
{
class CPortal;
class CSector;
class CHOM;

struct _scissor : public Fbox2
{
    float depth = 0.f;
};

class CPortal : public IRender_Portal
#ifdef DEBUG
                ,
                public pureRender
#endif
{
public:
    using Poly = svector<Fvector, 6>;
    struct level_portal_data_t
    {
        u16 sector_front;
        u16 sector_back;
        Poly vertices;
    };

private:
    Poly poly;
    CSector *pFace, *pBack;

public:
    Fplane P;
    Fsphere S;
    u32 marker = 0xffffffff;
    BOOL bDualRender = FALSE;

    void setup(const level_portal_data_t& data, const xr_vector<CSector*>& sectors);

    Poly& getPoly() { return poly; }
    CSector* Back() { return pBack; }
    CSector* Front() { return pFace; }
    CSector* getSector(CSector* pFrom) { return pFrom == pFace ? pBack : pFace; }
    CSector* getSectorFacing(const Fvector& V)
    {
        if (P.classify(V) > 0)
            return pFace;
        else
            return pBack;
    }
    CSector* getSectorBack(const Fvector& V)
    {
        if (P.classify(V) > 0)
            return pBack;
        else
            return pFace;
    }
    float distance(const Fvector& V) { return _abs(P.classify(V)); }

    CPortal();
    virtual ~CPortal();

#ifdef DEBUG
    virtual void OnRender();
#endif
};

class dxRender_Visual;

class CSector : public IRender_Sector
{
public:
    struct level_sector_data_t
    {
        xr_vector<u32> portals_id;
        u32 root_id;
    };

protected:
    dxRender_Visual* m_root;

public:
    xr_vector<CPortal*> m_portals;
    xr_vector<CFrustum> r_frustums;
    xr_vector<_scissor> r_scissors;
    _scissor r_scissor_merged;
    u32 r_marker = 0;

    dxRender_Visual* root() { return m_root; }
    void setup(const level_sector_data_t& data, const xr_vector<CPortal*>& portals);

    CSector() { m_root = nullptr; }
    virtual ~CSector() = default;
};

class CPortalTraverser
{
public:
    enum
    {
        VQ_HOM = (1 << 0),
        VQ_SSA = (1 << 1),
        VQ_SCISSOR = (1 << 2),
        VQ_FADE = (1 << 3),
    };

public:
    u32 i_marker = 0xffffffff;
    u32 i_options = 0;
    Fvector i_vBase;
    Fmatrix i_mXFORM;
    Fmatrix i_mXFORM_01;
    CSector* i_start = nullptr;
    CHOM* i_hom = nullptr;
    u32 i_recurseDepth = 0;
    static constexpr u32 kMaxPortalRecurse = 256;
    xr_vector<CSector*> r_sectors;
    xr_vector<std::pair<CPortal*, float>> f_portals;

public:
    CPortalTraverser();
    void traverse(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options, CHOM* hom = nullptr);
    void traverse_sector(CSector* sector, CFrustum& F, _scissor& R);
    void fade_portal(CPortal* _p, float ssa);
};

extern CPortalTraverser PortalTraverser;
} // namespace xray::render::fg
