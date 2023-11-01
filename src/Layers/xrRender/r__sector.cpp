// Portal.cpp: implementation of the CPortal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "r__sector.h"
#include "Common/LevelStructure.hpp"
#include "xrEngine/xr_object.h"
#include "FBasicVisual.h"
#include "xrEngine/IGame_Persistent.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CPortal::CPortal()
{
#ifdef DEBUG
    Device.seqRender.Add(this, REG_PRIORITY_LOW - 1000);
#endif
}

CPortal::~CPortal()
{
#ifdef DEBUG
    Device.seqRender.Remove(this);
#endif
}

#ifdef DEBUG
void CPortal::OnRender()
{
    if (psDeviceFlags.is(rsOcclusionDraw))
    {
        VERIFY(poly.size());
        // draw rect
        static xr_vector<FVF::L> V;
        V.resize(poly.size()*3);
        Fvector vCenter = { 0.0f, 0.0f, 0.0f };
        static u32 portalColor = 0x800000FF;
        for (u32 k = 0; k < poly.size(); ++k)
        {
            vCenter.add(poly[k]);
            V[k * 3 + 1].set(poly[k], portalColor);

            if (k + 1 == poly.size())
                V[k * 3 + 2].set(poly[0], portalColor);
            else
                V[k * 3 + 2].set(poly[k + 1], portalColor);
        }

        vCenter.div((float)poly.size());

        for (u32 k = 0; k < poly.size(); ++k)
            V[k * 3].set(vCenter, portalColor);

        RCache.set_xform_world(Fidentity);
        // draw solid
        RCache.set_Shader(RImplementation.m_SelectionShader);
#ifndef USE_DX9 // when we don't have FFP support
        RCache.set_c("tfactor", float(color_get_R(portalColor)) / 255.f, float(color_get_G(portalColor)) / 255.f, \
            float(color_get_B(portalColor)) / 255.f, float(color_get_A(portalColor)) / 255.f);
#endif
        RCache.dbg_Draw(D3DPT_TRIANGLELIST, &*V.begin(), V.size() / 3);

        // draw wire
        V.resize(poly.size()+1); // SkyLoader: change vertex array for wire
        for (u32 k = 0; k < poly.size(); ++k)
            V[k].set(poly[k], portalColor);
        V.back().set(poly[0], portalColor);

        if (bDebug)
            RImplementation.rmNear(RCache);
        else
            Device.SetNearer(TRUE);

        RCache.set_Shader(RImplementation.m_WireShader);
#ifndef USE_DX9 // when we don't have FFP support
        RCache.set_c("tfactor", float(color_get_R(portalColor)) / 255.f, float(color_get_G(portalColor)) / 255.f, \
            float(color_get_B(portalColor)) / 255.f, float(color_get_A(portalColor)) / 255.f);
#endif
        RCache.dbg_Draw(D3DPT_LINESTRIP, &*V.begin(), V.size() - 1);
        if (bDebug)
            RImplementation.rmNormal(RCache);
        else
            Device.SetNearer(FALSE);
    }
}
#endif
//
void CPortal::setup(const level_portal_data_t& data, const xr_vector<CSector*>& sectors)
{
    const auto* V = data.vertices.cbegin();
    const auto vcnt = data.vertices.size();
    CSector* face = sectors[data.sector_front];
    CSector* back = sectors[data.sector_back];

    // calc sphere
    Fbox BB;
    BB.invalidate();
    for (int v = 0; v < vcnt; v++)
        BB.modify(V[v]);
    BB.getsphere(S.P, S.R);

    //
    poly.assign(V, vcnt);
    pFace = face;
    pBack = back;
    marker = 0xffffffff;

    Fvector N, T;
    N.set(0, 0, 0);

    FPU::m64r();
    u32 _cnt = 0;
    for (int i = 2; i < vcnt; i++)
    {
        T.mknormal_non_normalized(poly[0], poly[i - 1], poly[i]);
        float m = T.magnitude();
        if (m > EPS_S)
        {
            N.add(T.div(m));
            _cnt++;
        }
    }
    R_ASSERT2(_cnt, "Invalid portal detected");
    N.div(float(_cnt));
    P.build(poly[0], N);
    FPU::m24r();

    /*
    if (_abs(1-P.n.magnitude())<EPS)
    xrDebug::Fatal      (DEBUG_INFO,"Degenerated portal found at {%3.2f,%3.2f,%3.2f}.",VPUSH(poly[0]));
    */
}

void CSector::setup(const level_sector_data_t& data, const xr_vector<CPortal*> &portals)
{
    // Assign portal polygons
    const auto num_portals = data.portals_id.size();
    m_portals.resize(num_portals);
    for (int idx = 0; idx < num_portals; ++idx)
    {
        const auto ID = data.portals_id[idx];
        m_portals[idx] = portals[ID];
    }

    if (GEnv.isDedicatedServer)
        m_root = nullptr;
    else
    {
        // Assign visual
        m_root = static_cast<dxRender_Visual*>(RImplementation.getVisual(data.root_id));
    }
}
