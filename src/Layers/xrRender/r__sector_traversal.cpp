#include "stdafx.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "FVF.h"

CPortalTraverser::CPortalTraverser() { i_marker = 0xffffffff; }
#ifdef DEBUG
xr_vector<IRender_Sector*> dbg_sectors;
#endif

void CPortalTraverser::traverse(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options)
{
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
    if (f_portals.empty())
        return;

    // re-sort, back to front
    std::sort(f_portals.begin(), f_portals.end(), [this](const auto& p1, const auto& p2)
    {
        const float d1 = i_vBase.distance_to_sqr(p1.first->S.P);
        const float d2 = i_vBase.distance_to_sqr(p2.first->S.P);
        return d2 > d1; // descending, back to front
    });

    // calc poly-count
    u32 _pcount = 0;
    for (u32 _it = 0; _it < f_portals.size(); _it++)
        _pcount += f_portals[_it].first->getPoly().size() - 2;

    // fill buffers
    u32 _offset = 0;
    FVF::L* _v = (FVF::L*)RImplementation.Vertex.Lock(_pcount * 3, RImplementation.m_PortalFadeGeom.stride(), _offset);
    float ssaRange = r_ssaLOD_A - r_ssaLOD_B;
    Fvector _ambient_f = g_pGamePersistent->Environment().CurrentEnv.ambient;
    u32 _ambient = color_rgba_f(_ambient_f.x, _ambient_f.y, _ambient_f.z, 0);
    for (u32 _it = 0; _it < f_portals.size(); _it++)
    {
        std::pair<CPortal*, float>& fp = f_portals[_it];
        CPortal* _P = fp.first;
        float _ssa = fp.second;
        float ssaDiff = _ssa - r_ssaLOD_B;
        float ssaScale = ssaDiff / ssaRange;
        int iA = iFloor((1 - ssaScale) * 255.5f);
        clamp(iA, 0, 255);
        u32 _clr = subst_alpha(_ambient, u32(iA));

        // fill polys
        u32 _polys = _P->getPoly().size() - 2;
        for (u32 _pit = 0; _pit < _polys; _pit++)
        {
            _v->set(_P->getPoly()[0], _clr);
            _v++;
            _v->set(_P->getPoly()[_pit + 1], _clr);
            _v++;
            _v->set(_P->getPoly()[_pit + 2], _clr);
            _v++;
        }
    }
    RImplementation.Vertex.Unlock(_pcount * 3, RImplementation.m_PortalFadeGeom.stride());

    // render
    RCache.set_xform_world(Fidentity);
    RCache.set_Shader(RImplementation.m_PortalFadeShader);
    RCache.set_Geometry(RImplementation.m_PortalFadeGeom);
    RCache.set_CullMode(CULL_NONE);
    RCache.Render(D3DPT_TRIANGLELIST, _offset, _pcount);
    RCache.set_CullMode(CULL_CCW);

    // cleanup
    f_portals.clear();
}

#ifdef DEBUG
void CPortalTraverser::dbg_draw()
{
    RCache.OnFrameEnd();

    RCache.set_xform_world(Fidentity);
    RCache.set_xform_view(Fidentity);
    RCache.set_xform_project(Fidentity);
#ifndef USE_DX9 // when we don't have FFP support
    RCache.set_Shader(RImplementation.m_WireShader);
    RCache.set_c("tfactor", 1.f, 1.f, 1.f, 1.f);
#endif

    for (u32 s = 0; s < dbg_sectors.size(); s++)
    {
        CSector* S = (CSector*)dbg_sectors[s];
        FVF::L verts[5];
        Fbox2 bb = S->r_scissor_merged;
        bb.min.x = bb.min.x * 2 - 1;
        bb.max.x = bb.max.x * 2 - 1;
        bb.min.y = (1 - bb.min.y) * 2 - 1;
        bb.max.y = (1 - bb.max.y) * 2 - 1;

        verts[0].set(bb.min.x, bb.min.y, EPS, 0xffffffff);
        verts[1].set(bb.max.x, bb.min.y, EPS, 0xffffffff);
        verts[2].set(bb.max.x, bb.max.y, EPS, 0xffffffff);
        verts[3].set(bb.min.x, bb.max.y, EPS, 0xffffffff);
        verts[4].set(bb.min.x, bb.min.y, EPS, 0xffffffff);
        RCache.dbg_Draw(D3DPT_LINESTRIP, verts, 4);
    }
}
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
                if ((i_options & CPortalTraverser::VQ_HOM) && (!RImplementation.HOM.visible(*P)))
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

                // Cull by HOM (faster algo)
                if ((i_options & CPortalTraverser::VQ_HOM) &&
                    !RImplementation.HOM.visible(scissor, depth))
                {
                    continue;
                }
            }
        }
        else
        {
            scissor = R_scissor;

            // Cull by HOM (slower algo)
            if ((i_options & CPortalTraverser::VQ_HOM) && (!RImplementation.HOM.visible(*P)))
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
