#include "stdafx.h"

#include "FHierrarhyVisual.h"
#include "SkeletonCustom.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/IRenderable.h"

#include "FLOD.h"
#include "particlegroup.h"
#include "FTreeVisual.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

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
        auto &pass = *sh->passes[iPass];
        auto &map = mapMatrixPasses[sh->flags.iPriority / 2][iPass];

#ifdef USE_OGL
        auto &Nvs = map[pass.vs->sh];
        auto &Ngs = Nvs[pass.gs->sh];
        auto &Nps = Ngs[pass.ps->sh];
#elif defined(USE_DX10) || defined(USE_DX11)
        auto &Nvs = map[&*pass.vs];
        auto &Ngs = Nvs[pass.gs->sh];
        auto &Nps = Ngs[pass.ps->sh];
#else
        auto &Nvs = map[pass.vs->sh];
        auto &Nps = Nvs[pass.ps->sh];
#endif

#ifdef USE_DX11
        Nps.hs = pass.hs->sh;
        Nps.ds = pass.ds->sh;

        auto &Ncs = Nps.mapCS[pass.constants._get()];
#else
        auto &Ncs = Nps[pass.constants._get()];
#endif
        auto &Nstate = Ncs[&*pass.state];
        auto &Ntex = Nstate[pass.T._get()];
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
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
                        if (SSA > Ngs.ssa)
                        {
                            Ngs.ssa = SSA;
#endif
                            if (SSA > Nvs.ssa)
                            {
                                Nvs.ssa = SSA;
                            }
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
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
        auto &pass = *sh->passes[iPass];
        auto &map = mapNormalPasses[sh->flags.iPriority / 2][iPass];

#ifdef USE_OGL
        auto &Nvs = map[pass.vs->sh];
        auto &Ngs = Nvs[pass.gs->sh];
        auto &Nps = Ngs[pass.ps->sh];
#elif defined(USE_DX10) || defined(USE_DX11)
        auto &Nvs = map[&*pass.vs];
        auto &Ngs = Nvs[pass.gs->sh];
        auto &Nps = Ngs[pass.ps->sh];
#else
        auto &Nvs = map[pass.vs->sh];
        auto &Nps = Nvs[pass.ps->sh];
#endif

#ifdef USE_DX11
        Nps.hs = pass.hs->sh;
        Nps.ds = pass.ds->sh;

        auto &Ncs = Nps.mapCS[pass.constants._get()];
#else
        auto &Ncs = Nps[pass.constants._get()];
#endif
        auto &Nstate = Ncs[&*pass.state];
        auto &Ntex = Nstate[pass.T._get()];
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
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
                        if (SSA > Ngs.ssa)
                        {
                            Ngs.ssa = SSA;
#endif
                            if (SSA > Nvs.ssa)
                            {
                                Nvs.ssa = SSA;
                            }
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
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
        for (auto &it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (I._effect)
                add_leafs_Dynamic(I._effect);
            for (auto &pit : I._children_related)
                add_leafs_Dynamic(pit);
            for (auto &pit : I._children_free)
                add_leafs_Dynamic(pit);
        }
    }
        return;
    case MT_HIERRARHY:
    {
        // Add all children, doesn't perform any tests
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        for (auto &i : pV->children)
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
            for (auto &i : pV->children)
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
        for (auto &it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (I._effect)
                add_leafs_Dynamic(I._effect);
            for (auto &pit : I._children_related)
                add_leafs_Dynamic(pit);
            for (auto &pit : I._children_free)
                add_leafs_Dynamic(pit);
        }
    }
        return;
    case MT_HIERRARHY:
    {
        // Add all children, doesn't perform any tests
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        for (auto &i : pV->children)
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
        for (auto &i : pV->children)
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
            for (auto &i : pV->children)
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
        for (auto &it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (fcvPartial == VIS)
            {
                if (I._effect)
                    add_Dynamic(I._effect, planes);
                for (auto &pit : I._children_related)
                    add_Dynamic(pit, planes);
                for (auto &pit : I._children_free)
                    add_Dynamic(pit, planes);
            }
            else
            {
                if (I._effect)
                    add_leafs_Dynamic(I._effect);
                for (auto &pit : I._children_related)
                    add_leafs_Dynamic(pit);
                for (auto &pit : I._children_free)
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
            for (auto &i : pV->children)
                add_Dynamic(i, planes);
        }
        else
        {
            for (auto &i : pV->children)
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
            for (auto &i : pV->children)
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
        for (auto &it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (fcvPartial == VIS)
            {
                if (I._effect)
                    add_Dynamic(I._effect, planes);
                for (auto &pit : I._children_related)
                    add_Dynamic(pit, planes);
                for (auto &pit : I._children_free)
                    add_Dynamic(pit, planes);
            }
            else
            {
                if (I._effect)
                    add_leafs_Dynamic(I._effect);
                for (auto &pit : I._children_related)
                    add_leafs_Dynamic(pit);
                for (auto &pit : I._children_free)
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
            for (auto &i : pV->children)
                add_Static(i, planes);
        }
        else
        {
            for (auto &i : pV->children)
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
            for (auto &i : pV->children)
                add_Static(i, planes);
        }
        else
        {
            for (auto &i : pV->children)
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
            for (auto &i : pV->children)
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

// ============================= NEW MEMBERS ===========================

D3DXRenderBase::D3DXRenderBase()
{
    val_pObject = nullptr;
    val_pTransform = nullptr;
    val_bHUD = FALSE;
    val_bInvisible = FALSE;
    val_bRecordMP = FALSE;
    val_feedback = nullptr;
    val_feedback_breakp = 0;
    val_recorder = nullptr;
    marker = 0;
    r_pmask(true, true);
    b_loaded = FALSE;
    Resources = nullptr;
}

void D3DXRenderBase::Copy(IRender& _in) { *this = *(D3DXRenderBase*)&_in; }
void D3DXRenderBase::setGamma(float fGamma)
{
#ifndef USE_OGL
    m_Gamma.Gamma(fGamma);
#else
    UNUSED(fGamma);
#endif
}

void D3DXRenderBase::setBrightness(float fGamma)
{
#ifndef USE_OGL
    m_Gamma.Brightness(fGamma);
#else
    UNUSED(fGamma);
#endif
}

void D3DXRenderBase::setContrast(float fGamma)
{
#ifndef USE_OGL
    m_Gamma.Contrast(fGamma);
#else
    UNUSED(fGamma);
#endif
}

void D3DXRenderBase::updateGamma()
{
#ifndef USE_OGL
    m_Gamma.Update();
#endif
}

void D3DXRenderBase::OnDeviceDestroy(bool bKeepTextures)
{
    m_WireShader.destroy();
    m_SelectionShader.destroy();
    Resources->OnDeviceDestroy(bKeepTextures);
    RCache.OnDeviceDestroy();
}

void D3DXRenderBase::ValidateHW() { HW.Validate(); }
void D3DXRenderBase::DestroyHW()
{
    xr_delete(Resources);
    HW.DestroyDevice();
}

void D3DXRenderBase::Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
#if defined(DEBUG) && !defined(USE_OGL)
    _SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif // DEBUG

    Resources->reset_begin();
    Memory.mem_compact();
    HW.Reset();

#if defined(USE_OGL)
    dwWidth = psCurrentVidMode[0];
    dwHeight = psCurrentVidMode[1];
#elif defined(USE_DX10) || defined(USE_DX11)
    dwWidth = HW.m_ChainDesc.BufferDesc.Width;
    dwHeight = HW.m_ChainDesc.BufferDesc.Height;
#else //    USE_DX10
    dwWidth = HW.DevPP.BackBufferWidth;
    dwHeight = HW.DevPP.BackBufferHeight;
#endif //   USE_DX10

    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);
    Resources->reset_end();

#if defined(DEBUG) && !defined(USE_OGL)
    _SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif
}

void D3DXRenderBase::SetupStates()
{
    HW.Caps.Update();
#if defined(USE_OGL)
    // TODO: OGL: Implement SetupStates().
#elif defined(USE_DX10) || defined(USE_DX11)
    SSManager.SetMaxAnisotropy(ps_r__tf_Anisotropic);
    SSManager.SetMipLODBias(ps_r__tf_Mipbias);
#else //    USE_DX10
    for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
    {
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, ps_r__tf_Anisotropic));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *(LPDWORD)&ps_r__tf_Mipbias));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
    }
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_LOCALVIEWER, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE));
    if (psDeviceFlags.test(rsWireframe))
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));
    else
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));
    // ******************** Fog parameters
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGCOLOR, 0));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE));
    if (HW.Caps.bTableFog)
    {
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE));
    }
    else
    {
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR));
    }
#endif // USE_DX10
}

void D3DXRenderBase::OnDeviceCreate(const char* shName)
{
    // Signal everyone - device created
    RCache.OnDeviceCreate();
#ifndef USE_OGL
    m_Gamma.Update();
#endif
    Resources->OnDeviceCreate(shName);
    create();
    if (!GEnv.isDedicatedServer)
    {
        m_WireShader.create("editor\\wire");
        m_SelectionShader.create("editor\\selection");
        DUImpl.OnDeviceCreate();
    }
}

void D3DXRenderBase::Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
    HW.CreateDevice(hWnd);
#if defined(USE_OGL)
    dwWidth = psCurrentVidMode[0];
    dwHeight = psCurrentVidMode[1];
#elif defined(USE_DX10) || defined(USE_DX11)
    dwWidth = HW.m_ChainDesc.BufferDesc.Width;
    dwHeight = HW.m_ChainDesc.BufferDesc.Height;
#else
    dwWidth = HW.DevPP.BackBufferWidth;
    dwHeight = HW.DevPP.BackBufferHeight;
#endif
    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);
    Resources = new CResourceManager();
}

void D3DXRenderBase::SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF)
{
    HW.Caps.bForceGPU_SW = bForceGPU_SW;
    HW.Caps.bForceGPU_NonPure = bForceGPU_NonPure;
    HW.Caps.bForceGPU_REF = bForceGPU_REF;
}

void D3DXRenderBase::overdrawBegin()
{
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    //  TODO: DX10: Implement overdrawBegin
    VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
#else
    // Turn stenciling
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, 0));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, 0x00000000));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff));
    // Increment the stencil buffer for each pixel drawn
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT));
    if (1 == HW.Caps.SceneMode)
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP)); // Overdraw
    else
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT)); // ZB access
#endif
}

void D3DXRenderBase::overdrawEnd()
{
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    // TODO: DX10: Implement overdrawEnd
    VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
#else
    // Set up the stencil states
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, 0xff));
    // Set the background to black
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, color_xrgb(255, 0, 0), 0, 0));
    // Draw a rectangle wherever the count equal I
    RCache.OnFrameEnd();
    CHK_DX(HW.pDevice->SetFVF(FVF::F_TL));
    // Render gradients
    for (int I = 0; I < 12; I++)
    {
        u32 _c = I * 256 / 13;
        u32 c = color_xrgb(_c, _c, _c);
        FVF::TL pv[4];
        pv[0].set(float(0), float(Device.dwHeight), c, 0, 0);
        pv[1].set(float(0), float(0), c, 0, 0);
        pv[2].set(float(Device.dwWidth), float(Device.dwHeight), c, 0, 0);
        pv[3].set(float(Device.dwWidth), float(0), c, 0, 0);
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, I));
        CHK_DX(HW.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pv, sizeof(FVF::TL)));
    }
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE));
#endif // USE_DX10
}

void D3DXRenderBase::DeferredLoad(bool E) { Resources->DeferredLoad(E); }
void D3DXRenderBase::ResourcesDeferredUpload() { Resources->DeferredUpload(); }
void D3DXRenderBase::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
    if (Resources)
        Resources->_GetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);
}

void D3DXRenderBase::ResourcesStoreNecessaryTextures() { Resources->StoreNecessaryTextures(); }
void D3DXRenderBase::ResourcesDumpMemoryUsage() { Resources->_DumpMemoryUsage(); }
DeviceState D3DXRenderBase::GetDeviceState()
{
    HW.Validate();
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
//  TODO: DX10: Implement GetDeviceState
//  TODO: DX10: Implement DXGI_PRESENT_TEST testing
// VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
#else // USE_DX10
    HRESULT _hr = HW.pDevice->TestCooperativeLevel();
    if (FAILED(_hr))
    {
        // If the device was lost, do not render until we get it back
        if (D3DERR_DEVICELOST == _hr)
            return DeviceState::Lost;
        // Check if the device is ready to be reset
        if (D3DERR_DEVICENOTRESET == _hr)
            return DeviceState::NeedReset;
    }
#endif // USE_DX10
    return DeviceState::Normal;
}

bool D3DXRenderBase::GetForceGPU_REF() { return HW.Caps.bForceGPU_REF; }
u32 D3DXRenderBase::GetCacheStatPolys() { return RCache.stat.polys; }
void D3DXRenderBase::Begin()
{
#if !defined(USE_DX10) && !defined(USE_DX11) && !defined(USE_OGL)
    CHK_DX(HW.pDevice->BeginScene());
#endif
    RCache.OnFrameBegin();
    RCache.set_CullMode(CULL_CW);
    RCache.set_CullMode(CULL_CCW);
    if (HW.Caps.SceneMode)
        overdrawBegin();
}

void D3DXRenderBase::Clear()
{
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    HW.pContext->ClearDepthStencilView(RCache.get_ZB(), D3D_CLEAR_DEPTH | D3D_CLEAR_STENCIL, 1.0f, 0);
    if (psDeviceFlags.test(rsClearBB))
    {
        FLOAT ColorRGBA[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
    }
#else
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_ZBUFFER | (psDeviceFlags.test(rsClearBB) ? D3DCLEAR_TARGET : 0) |
        (HW.Caps.bStencil ? D3DCLEAR_STENCIL : 0), color_xrgb(0, 0, 0), 1, 0));
#endif
}

void DoAsyncScreenshot();

void D3DXRenderBase::End()
{
    VERIFY(HW.pDevice);
    if (HW.Caps.SceneMode)
        overdrawEnd();
    RCache.OnFrameEnd();
    DoAsyncScreenshot();
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    bool bUseVSync = psDeviceFlags.is(rsFullscreen) && psDeviceFlags.test(rsVSync); //xxx: weird tearing glitches when VSync turned on for windowed mode in DX10\11
    HW.m_pSwapChain->Present(bUseVSync ? 1 : 0, 0);
#else
    CHK_DX(HW.pDevice->EndScene());
    HW.pDevice->Present(nullptr, nullptr, nullptr, nullptr);
#endif
}

void D3DXRenderBase::ResourcesDestroyNecessaryTextures() { Resources->DestroyNecessaryTextures(); }
void D3DXRenderBase::ClearTarget()
{
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    FLOAT ColorRGBA[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
#else
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, color_xrgb(0, 0, 0), 1, 0));
#endif
}

void D3DXRenderBase::SetCacheXform(Fmatrix& mView, Fmatrix& mProject)
{
    RCache.set_xform_view(mView);
    RCache.set_xform_project(mProject);
}

bool D3DXRenderBase::HWSupportsShaderYUV2RGB()
{
    u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
    u32 v_need = CAP_VERSION(2, 0);
    return v_dev >= v_need;
}

void D3DXRenderBase::OnAssetsChanged()
{
    Resources->m_textures_description.UnLoad();
    Resources->m_textures_description.Load();
}

void D3DXRenderBase::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    BasicStats.FrameEnd();
    auto renderTotal = Device.GetStats().RenderTotal.result;
#define PPP(a) (100.f * float(a) / renderTotal)
    font.OutNext("*** RENDER:   %2.2fms", renderTotal);
    font.OutNext("Calc:         %2.2fms, %2.1f%%", BasicStats.Culling.result, PPP(BasicStats.Culling.result));
    font.OutNext("Skeletons:    %2.2fms, %d", BasicStats.Animation.result, BasicStats.Animation.count);
    font.OutNext("Primitives:   %2.2fms, %2.1f%%", BasicStats.Primitives.result, PPP(BasicStats.Primitives.result));
    font.OutNext("Wait-L:       %2.2fms", BasicStats.Wait.result);
    font.OutNext("Wait-S:       %2.2fms", BasicStats.WaitS.result);
    font.OutNext("Skinning:     %2.2fms", BasicStats.Skinning.result);
    font.OutNext("DT_Vis/Cnt:   %2.2fms/%d", BasicStats.DetailVisibility.result, BasicStats.DetailCount);
    font.OutNext("DT_Render:    %2.2fms", BasicStats.DetailRender.result);
    font.OutNext("DT_Cache:     %2.2fms", BasicStats.DetailCache.result);
    font.OutNext("Wallmarks:    %2.2fms, %d/%d - %d", BasicStats.Wallmarks.result, BasicStats.StaticWMCount,
        BasicStats.DynamicWMCount, BasicStats.WMTriCount);
    font.OutNext("Glows:        %2.2fms", BasicStats.Glows.result);
    font.OutNext("Lights:       %2.2fms, %d", BasicStats.Lights.result, BasicStats.Lights.count);
    font.OutNext("RT:           %2.2fms, %d", BasicStats.RenderTargets.result, BasicStats.RenderTargets.count);
    font.OutNext("HUD:          %2.2fms", BasicStats.HUD.result);
    font.OutNext("P_calc:       %2.2fms", BasicStats.Projectors.result);
    font.OutNext("S_calc:       %2.2fms", BasicStats.ShadowsCalc.result);
    font.OutNext("S_render:     %2.2fms, %d", BasicStats.ShadowsRender.result, BasicStats.ShadowsRender.count);
    u32 occQs = BasicStats.OcclusionQueries ? BasicStats.OcclusionQueries : 1;
    font.OutNext("Occ-query:    %03.1f", 100.f * f32(BasicStats.OcclusionCulled) / occQs);
    font.OutNext("- queries:    %u", BasicStats.OcclusionQueries);
    font.OutNext("- culled:     %u", BasicStats.OcclusionCulled);
#undef PPP
    font.OutSkip();
    const auto& rcstats = RCache.stat;
    font.OutNext("Vertices:     %d/%d", rcstats.verts, rcstats.calls ? rcstats.verts / rcstats.calls : 0);
    font.OutNext("Polygons:     %d/%d", rcstats.polys, rcstats.calls ? rcstats.polys / rcstats.calls : 0);
    font.OutNext("DIP/DP:       %d", rcstats.calls);
#ifdef DEBUG
    font.OutNext("SH/T/M/C:     %d/%d/%d/%d", rcstats.states, rcstats.textures, rcstats.matrices, rcstats.constants);
    font.OutNext("RT/PS/VS:     %d/%d/%d", rcstats.target_rt, rcstats.ps, rcstats.vs);
    font.OutNext("DECL/VB/IB:   %d/%d/%d", rcstats.decl, rcstats.vb, rcstats.ib);
#endif
    font.OutNext("XForms:       %d", rcstats.xforms);
    font.OutNext("Static:       %3.1f/%d", rcstats.r.s_static.verts / 1024.f, rcstats.r.s_static.dips);
    font.OutNext("Flora:        %3.1f/%d", rcstats.r.s_flora.verts / 1024.f, rcstats.r.s_flora.dips);
    font.OutNext("- lods:       %3.1f/%d", rcstats.r.s_flora_lods.verts / 1024.f, rcstats.r.s_flora_lods.dips);
    font.OutNext("Dynamic:      %3.1f/%d", rcstats.r.s_dynamic.verts / 1024.f, rcstats.r.s_dynamic.dips);
    font.OutNext("- sw:         %3.1f/%d", rcstats.r.s_dynamic_sw.verts / 1024.f, rcstats.r.s_dynamic_sw.dips);
    font.OutNext("- inst:       %3.1f/%d", rcstats.r.s_dynamic_inst.verts / 1024.f, rcstats.r.s_dynamic_inst.dips);
    font.OutNext("- 1B:         %3.1f/%d", rcstats.r.s_dynamic_1B.verts / 1024.f, rcstats.r.s_dynamic_1B.dips);
    font.OutNext("- 2B:         %3.1f/%d", rcstats.r.s_dynamic_2B.verts / 1024.f, rcstats.r.s_dynamic_2B.dips);
    font.OutNext("- 3B:         %3.1f/%d", rcstats.r.s_dynamic_3B.verts / 1024.f, rcstats.r.s_dynamic_3B.dips);
    font.OutNext("- 4B:         %3.1f/%d", rcstats.r.s_dynamic_4B.verts / 1024.f, rcstats.r.s_dynamic_4B.dips);
    font.OutNext("Details:      %3.1f/%d", rcstats.r.s_details.verts / 1024.f, rcstats.r.s_details.dips);
    if (alert)
    {
        if (rcstats.verts > 500000)
            alert->Print(font, "Verts     > 500k: %d", rcstats.verts);
        if (rcstats.calls > 1000)
            alert->Print(font, "DIP/DP    > 1k:   %d", rcstats.calls);
        if (BasicStats.DetailCount > 1000)
            alert->Print(font, "DT_count  > 1000: %u", BasicStats.DetailCount);
    }
    BasicStats.FrameStart();
}
