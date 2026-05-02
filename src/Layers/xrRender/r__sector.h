// Portal.h: interface for the CPortal class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/_fbox2.h"

namespace CDB { class MODEL; }
class xrXRC;

namespace xray::render::fg
{
class CPortal;
class CSector;


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

    void setup(const level_portal_data_t& data, const xr_vector<CSector*>& sectors);

    CSector* getSectorFacing(const Fvector& V)
    {
        if (P.classify(V) > 0)
            return pFace;
        else
            return pBack;
    }
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
    dxRender_Visual* root() { return m_root; }
    void setup(const level_sector_data_t& data);

    CSector() { m_root = nullptr; }
    virtual ~CSector() = default;
};
} // namespace xray::render::fg
