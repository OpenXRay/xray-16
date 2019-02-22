#include "stdafx.h"

#include "FHierrarhyVisual.h"
#include "SkeletonCustom.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/IRenderable.h"

#include "FLOD.h"
#include "ParticleGroup.h"
#include "FTreeVisual.h"

using namespace R_dsgraph;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene graph actual insertion and sorting ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
float r_ssaDISCARD;
float r_ssaDONTSORT;
float r_ssaLOD_A, r_ssaLOD_B;
float r_ssaGLOD_start, r_ssaGLOD_end;
float r_ssaHZBvsTEX;

ICF float CalcSSA(float& distSQ, Fvector& C, dxRender_Visual* V)
{
    float R = V->vis.sphere.R + 0;
    distSQ = Device.vCameraPosition.distance_to_sqr(C) + EPS;
    return R / distSQ;
}
ICF float CalcSSA(float& distSQ, Fvector& C, float R)
{
    distSQ = Device.vCameraPosition.distance_to_sqr(C) + EPS;
    return R / distSQ;
}

void D3DXRenderBase::r_dsgraph_insert_dynamic(dxRender_Visual* pVisual, Fvector& Center)
{
    CRender& RI = RImplementation;

    if (pVisual->vis.marker == RI.marker)
        return;
    pVisual->vis.marker = RI.marker;

#if RENDER == R_R1
    if (RI.o.vis_intersect && (pVisual->vis.accept_frame != Device.dwFrame))
        return;
    pVisual->vis.accept_frame = Device.dwFrame;
#endif

    float distSQ;
    float SSA = CalcSSA(distSQ, Center, pVisual);
    if (SSA <= r_ssaDISCARD)
        return;

    // Distortive geometry should be marked and R2 special-cases it
    // a) Allow to optimize RT order
    // b) Should be rendered to special distort buffer in another pass
    VERIFY(pVisual->shader._get());
    ShaderElement* sh_d = &*pVisual->shader->E[4]; // 4=L_special
    if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
    {
        mapDistort.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh_d }))); // sh_d -> L_special
    }

    // Select shader
    ShaderElement* sh = RImplementation.rimp_select_sh_dynamic(pVisual, distSQ);
    if (nullptr == sh)
        return;
    if (!pmask[sh->flags.iPriority / 2])
        return;

    // HUD rendering
    if (RI.val_bHUD)
    {
        if (sh->flags.bStrictB2F)
            mapHUDSorted.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh })));
        else
            mapHUD      .emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh })));

#if RENDER != R_R1
        if (sh->flags.bEmissive)
            mapHUDEmissive.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh_d }))); // sh_d -> L_special
#endif
        return;
    }

// Shadows registering
#if RENDER == R_R1
    RI.L_Shadows->add_element(_MatrixItem{ SSA, RI.val_pObject, pVisual, *RI.val_pTransform });
#endif
    if (RI.val_bInvisible)
        return;

    // strict-sorting selection
    if (sh->flags.bStrictB2F)
    {
        mapSorted.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh })));
        return;
    }

#if RENDER != R_R1
    // Emissive geometry should be marked and R2 special-cases it
    // a) Allow to skeep already lit pixels
    // b) Allow to make them 100% lit and really bright
    // c) Should not cast shadows
    // d) Should be rendered to accumulation buffer in the second pass
    if (sh->flags.bEmissive)
    {
        mapEmissive.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh_d }))); // sh_d -> L_special
    }
    if (sh->flags.bWmark && pmask_wmark)
    {
        mapWmark.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, RI.val_pObject, pVisual, *RI.val_pTransform, sh })));
        return;
    }
#endif

    // Create common node
    // NOTE: Invisible elements exist only in R1
    _MatrixItem item = { SSA, RI.val_pObject, pVisual, *RI.val_pTransform };

    for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
    {
        auto& pass = *sh->passes[iPass];
        auto& map = mapMatrixPasses[sh->flags.iPriority / 2][iPass];

#ifdef USE_OGL
        auto& Nvs = map[pass.vs->sh];
        auto& Ngs = Nvs[pass.gs->sh];
        auto& Nps = Ngs[pass.ps->sh];
#elif defined(USE_DX10) || defined(USE_DX11)
        auto& Nvs = map[&*pass.vs];
        auto& Ngs = Nvs[pass.gs->sh];
        auto& Nps = Ngs[pass.ps->sh];
#else
        auto& Nvs = map[pass.vs->sh];
        auto& Nps = Nvs[pass.ps->sh];
#endif

#ifdef USE_DX11
        Nps.hs = pass.hs->sh;
        Nps.ds = pass.ds->sh;

        auto& Ncs = Nps.mapCS[pass.constants._get()];
#else
        auto& Ncs = Nps[pass.constants._get()];
#endif
        auto& Nstate = Ncs[&*pass.state];
        auto& Ntex = Nstate[pass.T._get()];
        Ntex.push_back(item);

        // Need to sort for HZB efficient use
        if (SSA > Ntex.ssa)
        {
            Ntex.ssa = SSA;
            if (SSA > Nstate.ssa)
            {
                Nstate.ssa = SSA;
                if (SSA > Ncs.ssa)
                {
                    Ncs.ssa = SSA;
#ifdef USE_DX11
                    if (SSA > Nps.mapCS.ssa)
                    {
                        Nps.mapCS.ssa = SSA;
#else
                    if (SSA > Nps.ssa)
                    {
                        Nps.ssa = SSA;
#endif
#ifndef USE_DX9
                        if (SSA > Ngs.ssa)
                        {
                            Ngs.ssa = SSA;
#endif
                            if (SSA > Nvs.ssa)
                            {
                                Nvs.ssa = SSA;
                            }
#ifndef USE_DX9
                        }
#endif
                    }
                }
            }
        }
    }

#if RENDER != R_R1
    if (val_recorder)
    {
        Fbox3 temp;
        Fmatrix& xf = *RI.val_pTransform;
        temp.xform(pVisual->vis.box, xf);
        val_recorder->push_back(temp);
    }
#endif
}

void D3DXRenderBase::r_dsgraph_insert_static(dxRender_Visual* pVisual)
{
    CRender& RI = RImplementation;

    if (pVisual->vis.marker == RI.marker)
        return;
    pVisual->vis.marker = RI.marker;

#if RENDER == R_R1
    if (RI.o.vis_intersect && (pVisual->vis.accept_frame != Device.dwFrame))
        return;
    pVisual->vis.accept_frame = Device.dwFrame;
#endif

    float distSQ;
    float SSA = CalcSSA(distSQ, pVisual->vis.sphere.P, pVisual);
    if (SSA <= r_ssaDISCARD)
        return;

    // Distortive geometry should be marked and R2 special-cases it
    // a) Allow to optimize RT order
    // b) Should be rendered to special distort buffer in another pass
    VERIFY(pVisual->shader._get());
    ShaderElement* sh_d = &*pVisual->shader->E[4]; // 4=L_special
    if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
    {
        mapDistort.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh_d }))); // sh_d -> L_special
    }

    // Select shader
    ShaderElement* sh = RImplementation.rimp_select_sh_static(pVisual, distSQ);
    if (nullptr == sh)
        return;
    if (!pmask[sh->flags.iPriority / 2])
        return;

    // strict-sorting selection
    if (sh->flags.bStrictB2F)
    {
        // TODO: Выяснить, почему в единственном месте параметр ssa не используется
        // Визуально различий не замечено
        mapSorted.emplace_back(std::make_pair(distSQ, _MatrixItemS({ /*0*/SSA, nullptr, pVisual, Fidentity, sh })));
        return;
    }

#if RENDER != R_R1
    // Emissive geometry should be marked and R2 special-cases it
    // a) Allow to skeep already lit pixels
    // b) Allow to make them 100% lit and really bright
    // c) Should not cast shadows
    // d) Should be rendered to accumulation buffer in the second pass
    if (sh->flags.bEmissive)
    {
        mapEmissive.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh_d }))); // sh_d -> L_special
    }
    if (sh->flags.bWmark && pmask_wmark)
    {
        mapWmark.emplace_back(std::make_pair(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh })));
        return;
    }
#endif

    if (val_feedback && counter_S == val_feedback_breakp)
        val_feedback->rfeedback_static(pVisual);

    counter_S++;

    _NormalItem item = { SSA, pVisual };

    for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
    {
        auto& pass = *sh->passes[iPass];
        auto& map = mapNormalPasses[sh->flags.iPriority / 2][iPass];

#ifdef USE_OGL
        auto& Nvs = map[pass.vs->sh];
        auto& Ngs = Nvs[pass.gs->sh];
        auto& Nps = Ngs[pass.ps->sh];
#elif defined(USE_DX10) || defined(USE_DX11)
        auto& Nvs = map[&*pass.vs];
        auto& Ngs = Nvs[pass.gs->sh];
        auto& Nps = Ngs[pass.ps->sh];
#else
        auto& Nvs = map[pass.vs->sh];
        auto& Nps = Nvs[pass.ps->sh];
#endif

#ifdef USE_DX11
        Nps.hs = pass.hs->sh;
        Nps.ds = pass.ds->sh;

        auto& Ncs = Nps.mapCS[pass.constants._get()];
#else
        auto& Ncs = Nps[pass.constants._get()];
#endif
        auto& Nstate = Ncs[&*pass.state];
        auto& Ntex = Nstate[pass.T._get()];
        Ntex.push_back(item);

        // Need to sort for HZB efficient use
        if (SSA > Ntex.ssa)
        {
            Ntex.ssa = SSA;
            if (SSA > Nstate.ssa)
            {
                Nstate.ssa = SSA;
                if (SSA > Ncs.ssa)
                {
                    Ncs.ssa = SSA;
#ifdef USE_DX11
                    if (SSA > Nps.mapCS.ssa)
                    {
                        Nps.mapCS.ssa = SSA;
#else
                    if (SSA > Nps.ssa)
                    {
                        Nps.ssa = SSA;
#endif
#ifndef USE_DX9
                        if (SSA > Ngs.ssa)
                        {
                            Ngs.ssa = SSA;
#endif
                            if (SSA > Nvs.ssa)
                            {
                                Nvs.ssa = SSA;
                            }
#ifndef USE_DX9
                        }
#endif
                    }
                }
            }
        }
    }

#if RENDER != R_R1
    if (val_recorder)
    {
        val_recorder->push_back(pVisual->vis.box);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::add_leafs_Dynamic(dxRender_Visual* pVisual)
{
    if (nullptr == pVisual)
        return;

    // Visual is 100% visible - simply add it
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Add all children, doesn't perform any tests
        PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
        for (auto& it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (I._effect)
                add_leafs_Dynamic(I._effect);
            for (auto& pit : I._children_related)
                add_leafs_Dynamic(pit);
            for (auto& pit : I._children_free)
                add_leafs_Dynamic(pit);
        }
    }
        return;
    case MT_HIERRARHY:
    {
        // Add all children, doesn't perform any tests
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        for (auto& i : pV->children)
        {
            i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                         // [use shader data from parent model, rather than it childrens]

            add_leafs_Dynamic(i);
        }
    }
        return;
    case MT_SKELETON_ANIM:
    case MT_SKELETON_RIGID:
    {
        // Add all children, doesn't perform any tests
        CKinematics* pV = (CKinematics*)pVisual;
        BOOL _use_lod = FALSE;
        if (pV->m_lod)
        {
            Fvector Tpos;
            float D;
            val_pTransform->transform_tiny(Tpos, pV->vis.sphere.P);
            float ssa = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
            if (ssa < r_ssaLOD_A)
                _use_lod = TRUE;
        }
        if (_use_lod)
        {
            add_leafs_Dynamic(pV->m_lod);
        }
        else
        {
            pV->CalculateBones(TRUE);
            pV->CalculateWallmarks(); //. bug?
            for (auto& i : pV->children)
            {
                i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                             // [use shader data from parent model, rather than it childrens]
                add_leafs_Dynamic(i);
            }
        }
    }
        return;
    default:
    {
        // General type of visual
        // Calculate distance to it's center
        Fvector Tpos;
        val_pTransform->transform_tiny(Tpos, pVisual->vis.sphere.P);
        r_dsgraph_insert_dynamic(pVisual, Tpos);
    }
        return;
    }
}

void CRender::add_leafs_Static(dxRender_Visual* pVisual)
{
    if (!HOM.visible(pVisual->vis))
        return;

    // Visual is 100% visible - simply add it
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Add all children, doesn't perform any tests
        PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
        for (auto& it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (I._effect)
                add_leafs_Dynamic(I._effect);
            for (auto& pit : I._children_related)
                add_leafs_Dynamic(pit);
            for (auto& pit : I._children_free)
                add_leafs_Dynamic(pit);
        }
    }
        return;
    case MT_HIERRARHY:
    {
        // Add all children, doesn't perform any tests
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        for (auto& i : pV->children)
        {
            i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                         // [use shader data from parent model, rather than it childrens]
            add_leafs_Static(i);
        }
    }
        return;
    case MT_SKELETON_ANIM:
    case MT_SKELETON_RIGID:
    {
        // Add all children, doesn't perform any tests
        CKinematics* pV = (CKinematics*)pVisual;
        pV->CalculateBones(TRUE);
        for (auto& i : pV->children)
        {
            i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                         // [use shader data from parent model, rather than it childrens]
            add_leafs_Static(i);
        }
    }
        return;
    case MT_LOD:
    {
        FLOD* pV = (FLOD*)pVisual;
        float D;
        float ssa = CalcSSA(D, pV->vis.sphere.P, pV);
        ssa *= pV->lod_factor;
        if (ssa < r_ssaLOD_A)
        {
            if (ssa < r_ssaDISCARD)
                return;
            mapLOD.emplace_back(std::make_pair(D, _LodItem({ ssa, pVisual })));
        }
#if RENDER != R_R1
        if (ssa > r_ssaLOD_B || phase == PHASE_SMAP)
#else
        if (ssa > r_ssaLOD_B)
#endif
        {
            // Add all children, doesn't perform any tests
            for (auto& i : pV->children)
            {
                i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                             // [use shader data from parent model, rather than it childrens]
                add_leafs_Static(i);
            }
        }
    }
        return;
    case MT_TREE_PM:
    case MT_TREE_ST:
    {
        // General type of visual
        r_dsgraph_insert_static(pVisual);
    }
        return;
    default:
    {
        // General type of visual
        r_dsgraph_insert_static(pVisual);
    }
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRender::add_Dynamic(dxRender_Visual* pVisual, u32 planes)
{
    // Check frustum visibility and calculate distance to visual's center
    Fvector Tpos; // transformed position
    EFC_Visible VIS;

    val_pTransform->transform_tiny(Tpos, pVisual->vis.sphere.P);
    VIS = View->testSphere(Tpos, pVisual->vis.sphere.R, planes);
    if (fcvNone == VIS)
        return FALSE;

    // If we get here visual is visible or partially visible
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Add all children, doesn't perform any tests
        PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
        for (auto& it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (fcvPartial == VIS)
            {
                if (I._effect)
                    add_Dynamic(I._effect, planes);
                for (auto& pit : I._children_related)
                    add_Dynamic(pit, planes);
                for (auto& pit : I._children_free)
                    add_Dynamic(pit, planes);
            }
            else
            {
                if (I._effect)
                    add_leafs_Dynamic(I._effect);
                for (auto& pit : I._children_related)
                    add_leafs_Dynamic(pit);
                for (auto& pit : I._children_free)
                    add_leafs_Dynamic(pit);
            }
        }
    }
    break;
    case MT_HIERRARHY:
    {
        // Add all children
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        if (fcvPartial == VIS)
        {
            for (auto& i : pV->children)
                add_Dynamic(i, planes);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_Dynamic(i);
        }
    }
    break;
    case MT_SKELETON_ANIM:
    case MT_SKELETON_RIGID:
    {
        // Add all children, doesn't perform any tests
        CKinematics* pV = (CKinematics*)pVisual;
        BOOL _use_lod = FALSE;
        if (pV->m_lod)
        {
            Fvector Tpos2;
            float D;
            val_pTransform->transform_tiny(Tpos2, pV->vis.sphere.P);
            float ssa = CalcSSA(D, Tpos2, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
            if (ssa < r_ssaLOD_A)
                _use_lod = TRUE;
        }
        if (_use_lod)
        {
            add_leafs_Dynamic(pV->m_lod);
        }
        else
        {
            pV->CalculateBones(TRUE);
            pV->CalculateWallmarks(); //. bug?
            for (auto& i : pV->children)
                add_leafs_Dynamic(i);
        }
    }
    break;
    default:
    {
        // General type of visual
        r_dsgraph_insert_dynamic(pVisual, Tpos);
    }
    break;
    }
    return TRUE;
}

void CRender::add_Static(dxRender_Visual* pVisual, u32 planes)
{
    // Check frustum visibility and calculate distance to visual's center
    EFC_Visible VIS;
    vis_data& vis = pVisual->vis;
    VIS = View->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
    if (fcvNone == VIS)
        return;

    if (!HOM.visible(vis))
        return;

    // If we get here visual is visible or partially visible
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Add all children, doesn't perform any tests
        PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
        for (auto& it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (fcvPartial == VIS)
            {
                if (I._effect)
                    add_Dynamic(I._effect, planes);
                for (auto& pit : I._children_related)
                    add_Dynamic(pit, planes);
                for (auto& pit : I._children_free)
                    add_Dynamic(pit, planes);
            }
            else
            {
                if (I._effect)
                    add_leafs_Dynamic(I._effect);
                for (auto& pit : I._children_related)
                    add_leafs_Dynamic(pit);
                for (auto& pit : I._children_free)
                    add_leafs_Dynamic(pit);
            }
        }
    }
    break;
    case MT_HIERRARHY:
    {
        // Add all children
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        if (fcvPartial == VIS)
        {
            for (auto& i : pV->children)
                add_Static(i, planes);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_Static(i);
        }
    }
    break;
    case MT_SKELETON_ANIM:
    case MT_SKELETON_RIGID:
    {
        // Add all children, doesn't perform any tests
        CKinematics* pV = (CKinematics*)pVisual;
        pV->CalculateBones(TRUE);
        if (fcvPartial == VIS)
        {
            for (auto& i : pV->children)
                add_Static(i, planes);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_Static(i);
        }
    }
    break;
    case MT_LOD:
    {
        FLOD* pV = (FLOD*)pVisual;
        float D;
        float ssa = CalcSSA(D, pV->vis.sphere.P, pV);
        ssa *= pV->lod_factor;
        if (ssa < r_ssaLOD_A)
        {
            if (ssa < r_ssaDISCARD)
                return;
            mapLOD.emplace_back(std::make_pair(D, _LodItem({ ssa, pVisual })));
        }
#if RENDER != R_R1
        if (ssa > r_ssaLOD_B || phase == PHASE_SMAP)
#else
        if (ssa > r_ssaLOD_B)
#endif
        {
            // Add all children, perform tests
            for (auto& i : pV->children)
                add_leafs_Static(i);
        }
    }
    break;
    case MT_TREE_ST:
    case MT_TREE_PM:
    {
        // General type of visual
        r_dsgraph_insert_static(pVisual);
    }
        return;
    default:
    {
        // General type of visual
        r_dsgraph_insert_static(pVisual);
    }
    break;
    }
}
