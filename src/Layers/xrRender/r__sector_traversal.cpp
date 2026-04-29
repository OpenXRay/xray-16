#include "stdafx.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "FVF.h"

namespace xray::render::fg
{
CPortalTraverser::CPortalTraverser() { i_marker = 0xffffffff; }
#ifdef DEBUG
xr_vector<IRender_Sector*> dbg_sectors;
#endif

void CPortalTraverser::traverse(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options)
{
    ZoneScoped;

    Fmatrix m_viewport_01 = {1.f / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -1.f / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.f / 2.f + 0 + 0, 1.f / 2.f + 0 + 0, 0.0f, 1.0f};

    if (options & VQ_FADE)
    {
        f_portals.clear();
        f_portals.reserve(16);
    }

    VERIFY(start);
    i_marker++;
    i_options = options;
    i_vBase = vBase;
    i_mXFORM = mXFORM;
    i_mXFORM_01.mul(m_viewport_01, mXFORM);
    i_start = (CSector*)start;
    r_sectors.clear();
    _scissor scissor;
    scissor.set(0, 0, 1, 1);
    scissor.depth = 0;
    traverse_sector(i_start, F, scissor);

    if (options & VQ_SCISSOR)
    {
        // dbg_sectors					= r_sectors;
        // merge scissor info
        for (u32 s = 0; s < r_sectors.size(); s++)
        {
            CSector* S = (CSector*)r_sectors[s];
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
extern float r_ssaDISCARD;
extern float r_ssaLOD_A, r_ssaLOD_B;
void CPortalTraverser::fade_render()
{
    f_portals.clear();
}

#ifdef DEBUG
void CPortalTraverser::dbg_draw() {}
#endif

void CPortalTraverser::traverse_sector(CSector* sector, CFrustum& F, _scissor& R_scissor)
{
    // Register traversal process sector
    if (sector->r_marker != i_marker)
    {
        sector->r_marker = i_marker;
        r_sectors.push_back(sector);
        sector->r_frustums.clear();
        sector->r_scissors.clear();
    }
    sector->r_frustums.push_back(F);
    sector->r_scissors.push_back(R_scissor);

    // Search visible portals and go through them
    sPoly S, D;
    for (u32 I = 0; I < sector->m_portals.size(); I++)
    {
        if (sector->m_portals[I]->marker == i_marker)
            continue;

        CPortal* PORTAL = sector->m_portals[I];
        CSector* pSector;

        // Select sector (allow intersecting portals to be finely classified)
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

        // Early-out sphere
        if (!F.testSphere_dirty(PORTAL->S.P, PORTAL->S.R))
            continue;

        // SSA  (if required)
        if (i_options & CPortalTraverser::VQ_SSA)
        {
            Fvector dir2portal;
            dir2portal.sub(PORTAL->S.P, i_vBase);
            float R = PORTAL->S.R;
            float distSQ = dir2portal.square_magnitude();
            float ssa = R * R / distSQ;
            dir2portal.div(_sqrt(distSQ));
            ssa *= _abs(PORTAL->P.n.dotproduct(dir2portal));
            if (ssa < r_ssaDISCARD)
                continue;

            if (i_options & CPortalTraverser::VQ_FADE)
            {
                if (ssa < r_ssaLOD_A)
                    fade_portal(PORTAL, ssa);
                if (ssa < r_ssaLOD_B)
                    continue;
            }
        }

        // Clip by frustum
        CPortal::Poly& POLY = PORTAL->getPoly();
        S.assign(&*POLY.begin(), POLY.size());
        D.clear();
        sPoly* P = F.ClipPoly(S, D);
        if (nullptr == P)
            continue;

        // Scissor and optimized HOM-testing
        _scissor scissor;
        if (i_options & CPortalTraverser::VQ_SCISSOR && (!PORTAL->bDualRender))
        {
            // Build scissor rectangle in projection-space
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
            // Msg  ("bb(%s): (%f,%f)-(%f,%f), d=%f", PORTAL->bDualRender?"true":"false",bb.min.x, bb.min.y, bb.max.x,
            // bb.max.y,depth);
            if (depth < EPS)
            {
                scissor = R_scissor;

                // Cull by HOM (slower algo)
                if (false)
                    continue;
            }
            else
            {
                // perform intersection (this is just to be sure, it is probably clipped in 3D already)
                if (bb.min.x > R_scissor.min.x)
                    scissor.min.x = bb.min.x;
                else
                    scissor.min.x = R_scissor.min.x;
                if (bb.min.y > R_scissor.min.y)
                    scissor.min.y = bb.min.y;
                else
                    scissor.min.y = R_scissor.min.y;
                if (bb.max.x < R_scissor.max.x)
                    scissor.max.x = bb.max.x;
                else
                    scissor.max.x = R_scissor.max.x;
                if (bb.max.y < R_scissor.max.y)
                    scissor.max.y = bb.max.y;
                else
                    scissor.max.y = R_scissor.max.y;
                scissor.depth = depth;

                // Msg("scissor: (%f,%f)-(%f,%f)", scissor.min.x, scissor.min.y, scissor.max.x, scissor.max.y);
                //  Check if box is non-empty
                if (scissor.min.x >= scissor.max.x)
                    continue;
                if (scissor.min.y >= scissor.max.y)
                    continue;

                if (false)
                {
                    continue;
                }
            }
        }
        else
        {
            scissor = R_scissor;

            if (false)
                continue;
        }

        // Create _new_ frustum and recurse
        CFrustum Clip;
        Clip.CreateFromPortal(P, PORTAL->P.n, i_vBase, i_mXFORM);
        PORTAL->marker = i_marker;
        PORTAL->bDualRender = FALSE;
        traverse_sector(pSector, Clip, scissor);
    }
}
} // namespace xray::render::fg
