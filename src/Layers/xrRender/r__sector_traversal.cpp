#include "stdafx.h"
#include "r__sector.h"
#include "Layers/xrRender/HOM.h"

namespace xray::render::fg
{
CPortalTraverser PortalTraverser;

CPortalTraverser::CPortalTraverser() { i_marker = 0xffffffff; }

void CPortalTraverser::traverse(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options, CHOM* hom)
{
    ZoneScopedN("PortalTraverser::traverse");

    Fmatrix m_viewport_01 = {
        1.f / 2.f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.f / 2.f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        1.f / 2.f, 1.f / 2.f, 0.0f, 1.0f};

    if (options & VQ_FADE)
    {
        f_portals.clear();
        f_portals.reserve(16);
    }

    if (!start)
    {
        r_sectors.clear();
        return;
    }

    i_marker++;
    if (i_marker == 0)
        i_marker = 1;
    i_options = options;
    i_vBase = vBase;
    i_mXFORM = mXFORM;
    i_mXFORM_01.mul(m_viewport_01, mXFORM);
    i_start = static_cast<CSector*>(start);
    i_hom = hom;
    i_recurseDepth = 0;
    r_sectors.clear();

    _scissor scissor;
    scissor.set(0, 0, 1, 1);
    scissor.depth = 0;
    traverse_sector(i_start, F, scissor);

    if (options & VQ_SCISSOR)
    {
        for (u32 s = 0; s < r_sectors.size(); s++)
        {
            CSector* S = r_sectors[s];
            S->r_scissor_merged.invalidate();
            S->r_scissor_merged.depth = flt_max;
            for (u32 it = 0; it < S->r_scissors.size(); it++)
            {
                S->r_scissor_merged.merge(S->r_scissors[it]);
                if (S->r_scissors[it].depth < S->r_scissor_merged.depth)
                    S->r_scissor_merged.depth = S->r_scissors[it].depth;
            }
        }
    }
}

void CPortalTraverser::fade_portal(CPortal* _p, float ssa) { f_portals.emplace_back(_p, ssa); }

void CPortalTraverser::traverse_sector(CSector* sector, CFrustum& F, _scissor& R_scissor)
{
    if (!sector)
        return;
    if (i_recurseDepth >= kMaxPortalRecurse)
        return;
    ++i_recurseDepth;

    if (sector->r_marker != i_marker)
    {
        sector->r_marker = i_marker;
        r_sectors.push_back(sector);
        sector->r_frustums.clear();
        sector->r_scissors.clear();
    }
    sector->r_frustums.push_back(F);
    sector->r_scissors.push_back(R_scissor);

    sPoly S, D;
    for (u32 I = 0; I < sector->m_portals.size(); I++)
    {
        if (!sector->m_portals[I])
            continue;
        if (sector->m_portals[I]->marker == i_marker)
            continue;

        CPortal* PORTAL = sector->m_portals[I];
        CSector* pSector = nullptr;

        if (PORTAL->bDualRender)
        {
            pSector = PORTAL->getSector(sector);
        }
        else
        {
            pSector = PORTAL->getSectorBack(i_vBase);
            if (pSector == sector)
                continue;
            if (pSector == i_start)
                continue;
        }

        if (!F.testSphere_dirty(PORTAL->S.P, PORTAL->S.R))
            continue;

        CPortal::Poly& POLY = PORTAL->getPoly();
        S.assign(&*POLY.begin(), POLY.size());
        D.clear();
        sPoly* P = F.ClipPoly(S, D);
        if (!P)
            continue;

        _scissor scissor;
        if ((i_options & CPortalTraverser::VQ_SCISSOR) && (!PORTAL->bDualRender))
        {
            Fbox2 bb;
            bb.invalidate();
            float depth = flt_max;
            sPoly& p = *P;
            for (u32 vit = 0; vit < p.size(); vit++)
            {
                Fvector4 t;
                Fmatrix& M = i_mXFORM_01;
                Fvector& v = p[vit];

                t.x = v.x * M._11 + v.y * M._21 + v.z * M._31 + M._41;
                t.y = v.x * M._12 + v.y * M._22 + v.z * M._32 + M._42;
                t.z = v.x * M._13 + v.y * M._23 + v.z * M._33 + M._43;
                t.w = v.x * M._14 + v.y * M._24 + v.z * M._34 + M._44;
                t.mul(1.f / t.w);

                if (t.x < bb.min.x)
                    bb.min.x = t.x;
                if (t.x > bb.max.x)
                    bb.max.x = t.x;
                if (t.y < bb.min.y)
                    bb.min.y = t.y;
                if (t.y > bb.max.y)
                    bb.max.y = t.y;
                if (t.z < depth)
                    depth = t.z;
            }

            if (depth < EPS)
            {
                scissor = R_scissor;
                if ((i_options & CPortalTraverser::VQ_HOM) && i_hom && (!i_hom->visible(*P)))
                    continue;
            }
            else
            {
                scissor.min.x = _max(bb.min.x, R_scissor.min.x);
                scissor.min.y = _max(bb.min.y, R_scissor.min.y);
                scissor.max.x = _min(bb.max.x, R_scissor.max.x);
                scissor.max.y = _min(bb.max.y, R_scissor.max.y);
                scissor.depth = depth;

                if (scissor.min.x >= scissor.max.x)
                    continue;
                if (scissor.min.y >= scissor.max.y)
                    continue;

                if ((i_options & CPortalTraverser::VQ_HOM) && i_hom &&
                    !i_hom->visible(scissor, depth))
                    continue;
            }
        }
        else
        {
            scissor = R_scissor;
            if ((i_options & CPortalTraverser::VQ_HOM) && i_hom && (!i_hom->visible(*P)))
                continue;
        }

        CFrustum Clip;
        Clip.CreateFromPortal(P, PORTAL->P.n, i_vBase, i_mXFORM);
        PORTAL->marker = i_marker;
        PORTAL->bDualRender = FALSE;
        traverse_sector(pSector, Clip, scissor);
    }
    --i_recurseDepth;
}
} // namespace xray::render::fg
