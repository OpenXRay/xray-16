#include "stdafx.h"

#include "FHierrarhyVisual.h"
#include "SkeletonCustom.h"
#include "xrCore/Threading/ParallelFor.hpp"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/IRenderable.h"
#include "xrEngine/xr_object.h"

#include "FLOD.h"
#include "LightTrack.h"
#include "ParticleGroup.h"
#include "FTreeVisual.h"

namespace xray::render::RENDER_NAMESPACE
{
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

void R_dsgraph_structure::insert_dynamic(IRenderable* root, dxRender_Visual* pVisual, Fmatrix& xform, Fvector& Center)
{
    ZoneScoped;

    CRender& RI = RImplementation;

    if (pVisual->vis.marker[context_id] == marker)
        return;
    pVisual->vis.marker[context_id] = marker;

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
    const Shader* vis_sh = pVisual->shader._get();
    ShaderElement* sh_d = vis_sh ? vis_sh->E[4]._get() : nullptr; // 4=L_special
    if (sh_d)
    {
        if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && o.pmask[sh_d->flags.iPriority / 2])
        {
            mapDistort.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh_d })); // sh_d -> L_special
        }
    }

    // Select shader
    ShaderElement* sh = RImplementation.rimp_select_sh_dynamic(pVisual, distSQ, o.phase);
    if (nullptr == sh)
        return;
    if (!o.pmask[sh->flags.iPriority / 2])
        return;

    // HUD rendering
    if (root && root->renderable_HUD())
    {
        if (sh->flags.bStrictB2F)
        {
            mapHUDSorted.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh }));
            return;
        }
        mapHUD.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh }));

#if RENDER != R_R1
        if (sh->flags.bEmissive && sh_d)
            mapHUDEmissive.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh_d })); // sh_d -> L_special
#endif
        return;
    }

// Shadows registering
#if RENDER == R_R1
    RI.L_Shadows->add_element(_MatrixItem{ SSA, root, pVisual, xform });
#endif
    if (root && root->renderable_Invisible())
        return;

    // strict-sorting selection
    if (sh->flags.bStrictB2F)
    {
        mapSorted.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh }));
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
        mapEmissive.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh_d })); // sh_d -> L_special
    }
    if (sh->flags.bWmark && o.pmask_wmark)
    {
        mapWmark.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh }));
        return;
    }
#endif

    for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
    {
        SPass* pass = sh->passes[iPass]._get();
        mapMatrix_T& map = mapMatrixPasses[sh->flags.iPriority / 2][iPass];
        mapMatrixItems& matrixItems = map[pass];

        // Create common node
        // NOTE: Invisible elements exist only in R1
        matrixItems.emplace_back(_MatrixItem{ SSA, root, pVisual, xform });

        // Need to sort for HZB efficient use
        if (SSA > matrixItems.ssa)
        {
            matrixItems.ssa = SSA;
        }
    }

#if RENDER != R_R1
    if (val_recorder)
    {
        Fbox3 temp;
        temp.xform(pVisual->vis.box, xform);
        val_recorder->push_back(temp);
    }
#endif
}

void R_dsgraph_structure::insert_static(dxRender_Visual* pVisual)
{
    ZoneScoped;

    CRender& RI = RImplementation;

    if (pVisual->vis.marker[context_id] == marker)
        return;
    pVisual->vis.marker[context_id] = marker;

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
    const Shader* vis_sh = pVisual->shader._get();
    ShaderElement* sh_d = vis_sh ? vis_sh->E[4]._get() : nullptr; // 4=L_special
    if (sh_d)
    {
        if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && o.pmask[sh_d->flags.iPriority / 2])
        {
            mapDistort.insert_anyway(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh_d })); // sh_d -> L_special
        }
    }

    // Select shader
    ShaderElement* sh = RImplementation.rimp_select_sh_static(pVisual, distSQ, o.phase);
    if (nullptr == sh)
        return;
    if (!o.pmask[sh->flags.iPriority / 2])
        return;

    // strict-sorting selection
    if (sh->flags.bStrictB2F)
    {
        // TODO: Выяснить, почему в единственном месте параметр ssa не используется
        // Визуально различий не замечено
        mapSorted.insert_anyway(distSQ, _MatrixItemS({ /*0*/SSA, nullptr, pVisual, Fidentity, sh }));
        return;
    }

#if RENDER != R_R1
    // Emissive geometry should be marked and R2 special-cases it
    // a) Allow to skeep already lit pixels
    // b) Allow to make them 100% lit and really bright
    // c) Should not cast shadows
    // d) Should be rendered to accumulation buffer in the second pass
    if (sh->flags.bEmissive && sh_d)
    {
        mapEmissive.insert_anyway(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh_d })); // sh_d -> L_special
    }
    if (sh->flags.bWmark && o.pmask_wmark)
    {
        mapWmark.insert_anyway(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh }));
        return;
    }
#endif

    if (val_feedback && counter_S == val_feedback_breakp)
        val_feedback->rfeedback_static(pVisual);

    counter_S++;

    for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
    {
        SPass* pass = sh->passes[iPass]._get();
        mapNormal_T& map = mapNormalPasses[sh->flags.iPriority / 2][iPass];
        mapNormalItems& normalItems = map[pass];

        normalItems.emplace_back(_NormalItem{ SSA, pVisual });

        // Need to sort for HZB efficient use
        if (SSA > normalItems.ssa)
        {
            normalItems.ssa = SSA;
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
void R_dsgraph_structure::add_leafs_dynamic(IRenderable* root, dxRender_Visual* pVisual, Fmatrix& xform)
{
    ZoneScoped;

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
                add_leafs_dynamic(root, I._effect, xform);
            for (auto& pit : I._children_related)
                add_leafs_dynamic(root, pit, xform);
            for (auto& pit : I._children_free)
                add_leafs_dynamic(root, pit, xform);
        }
    }
        return;
    case MT_HIERRARHY:
    {
        // Add all children, doesn't perform any tests
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        for (auto& i : pV->children)
        {
            //i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                         // [use shader data from parent model, rather than it childrens]

            add_leafs_dynamic(root, i, xform);
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
            xform.transform_tiny(Tpos, pV->vis.sphere.P);
            float ssa = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere
            if (ssa < r_ssaLOD_A)
                _use_lod = TRUE;
        }
        if (_use_lod)
        {
            add_leafs_dynamic(root, pV->m_lod, xform);
        }
        else
        {
            pV->CalculateBones(TRUE);
            if (o.phase == CRender::PHASE_NORMAL)
            {
                pV->CalculateWallmarks(root ? root->renderable_HUD() : false); //. bug?
            }
            for (auto& i : pV->children)
            {
                //i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                             // [use shader data from parent model, rather than it childrens]
                add_leafs_dynamic(root, i, xform);
            }
        }
    }
        return;
    default:
    {
        // General type of visual
        // Calculate distance to it's center
        Fvector Tpos;
        xform.transform_tiny(Tpos, pVisual->vis.sphere.P);
        insert_dynamic(root, pVisual, xform, Tpos);
    }
        return;
    }
}

void R_dsgraph_structure::add_leafs_static(dxRender_Visual* pVisual)
{
    ZoneScoped;

    if (o.use_hom && !RImplementation.HOM.visible(pVisual->vis))
        return;

    // Visual is 100% visible - simply add it
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Xottab_DUTY: for dynamic objects we need matrixб
        // which is nullptr, when we use add_leafs_Static
        Log("Dynamic particles added via static procedure. Please, contact Xottab_DUTY and tell him about the issue.");
        NODEFAULT;

        // Add all children, doesn't perform any tests
        /*PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
        for (auto& it : pG->items)
        {
            PS::CParticleGroup::SItem& I = it;
            if (I._effect)
                add_leafs_Dynamic(I._effect);
            for (auto& pit : I._children_related)
                add_leafs_Dynamic(pit);
            for (auto& pit : I._children_free)
                add_leafs_Dynamic(pit);
        }*/
    }
    return;
    case MT_HIERRARHY:
    {
        // Add all children, doesn't perform any tests
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        for (auto& i : pV->children)
        {
            //i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                         // [use shader data from parent model, rather than it childrens]
            add_leafs_static(i);
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
            //i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                         // [use shader data from parent model, rather than it childrens]
            add_leafs_static(i);
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
            mapLOD.insert_anyway(D, _LodItem({ ssa, pVisual }));
        }
#if RENDER != R_R1
        if (ssa > r_ssaLOD_B || o.phase == CRender::PHASE_SMAP)
#else
        if (ssa > r_ssaLOD_B)
#endif
        {
            // Add all children, doesn't perform any tests
            for (auto& i : pV->children)
            {
                //i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                             // [use shader data from parent model, rather than it childrens]
                add_leafs_static(i);
            }
        }
    }
    return;
    case MT_TREE_PM:
    case MT_TREE_ST:
    {
        // General type of visual
        insert_static(pVisual);
    }
    return;
    default:
    {
        // General type of visual
        insert_static(pVisual);
    }
    return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* Xottab_DUTY: this function is only called from add_Static,
 * but we need a matrix, which is nullptr at this point
BOOL R_dsgraph_structure::add_Dynamic(dxRender_Visual* pVisual, u32 planes) // normal processing
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
            pV->CalculateWallmarks(val_pObject ? val_pObject->renderable_HUD() : false); //. bug?
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
}*/

void R_dsgraph_structure::add_static(dxRender_Visual* pVisual, const CFrustum& view, u32 planes)
{
    ZoneScoped;

    vis_data& vis = pVisual->vis;

    // Check frustum visibility and calculate distance to visual's center
    const EFC_Visible VIS = view.testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
    if (fcvNone == VIS)
        return;

    if (o.use_hom && !RImplementation.HOM.visible(vis))
        return;

    // If we get here visual is visible or partially visible
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Xottab_DUTY: for dynamic objects we need matrix,
        // which is nullptr, when we use add_Static
        Log("Dynamic particles added via static procedure. Please, contact Xottab_DUTY and tell him about the issue.");
        NODEFAULT;

        // Add all children, doesn't perform any tests
        /*PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
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
        }*/
    }
    break;
    case MT_HIERRARHY:
    {
        // Add all children
        FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;
        if (fcvPartial == VIS)
        {
            for (auto& i : pV->children)
                add_static(i, view, planes);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_static(i);
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
                add_static(i, view, planes);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_static(i);
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
            mapLOD.insert_anyway(D, _LodItem({ ssa, pVisual }));
        }
#if RENDER != R_R1
        if (ssa > r_ssaLOD_B || o.phase == CRender::PHASE_SMAP)
#else
        if (ssa > r_ssaLOD_B)
#endif
        {
            // Add all children, perform tests
            for (auto& i : pV->children)
                add_leafs_static(i);
        }
    }
    break;
    case MT_TREE_ST:
    case MT_TREE_PM:
    {
        // General type of visual
        insert_static(pVisual);
    }
        return;
    default:
    {
        // General type of visual
        insert_static(pVisual);
    }
    break;
    }
}

void R_dsgraph_structure::load(const xr_vector<CSector::level_sector_data_t>& sectors_data,
    const xr_vector<CPortal::level_portal_data_t>& portals_data)
{
    ZoneScoped;

    const auto portals_count = portals_data.size();
    const auto sectors_count = sectors_data.size();

    Sectors.resize(sectors_count);
    Portals.resize(portals_count);

    for (u32 idx = 0; idx < portals_count; ++idx)
    {
        auto* portal = xr_new<CPortal>();
        Portals[idx] = portal;
    }

    for (u32 idx = 0; idx < sectors_count; ++idx)
    {
        auto* sector = xr_new<CSector>();

        sector->unique_id = static_cast<IRender_Sector::sector_id_t>(idx);
        sector->setup(sectors_data[idx], Portals);
        Sectors[idx] = sector;
    }

    for (u32 idx = 0; idx < portals_count; ++idx)
    {
        auto* portal = static_cast<CPortal*>(Portals[idx]);

        portal->setup(portals_data[idx], Sectors);
    }
}

void R_dsgraph_structure::unload()
{
    for (auto* sector : Sectors)
        xr_delete(sector);
    Sectors.clear();

    for (auto* portal : Portals)
        xr_delete(portal);
    Portals.clear();
}


// Pose a bone hierarchy in one skeleton (full-detail shadow stand-in) to match another,
// differently-proportioned skeleton (the actor's own live, currently-worn-outfit visual)
// by bone name. Copying accumulated world transforms directly (first attempt) warped
// badly during animation, because it silently carries the SOURCE model's bone lengths
// into the TARGET model's geometry — the two body meshes share the same rig topology but
// not necessarily identical bone proportions, and this error compounds down each bone
// chain, so it's small near bind pose (idle) and large at the chain's far end (head,
// hands) once the pose actually moves (walking) — matching what was observed in-game
// 2026-07-27. Fix: decompose the source bone's LOCAL (parent-relative) transform, keep
// only its ROTATION, and recombine with the TARGET's OWN bind-pose translation (bone
// length) before accumulating through the TARGET's own hierarchy — proper retargeting,
// not a flat world-transform copy.
static void retarget_bone_recursive(IKinematics* full_kin, IKinematics* live_kin, CBoneData& full_bd,
    const Fmatrix& full_parent_world, const Fmatrix& live_parent_world, bool live_parent_valid)
{
    const u16 full_id = full_bd.GetSelfID();
    const u16 live_id = live_kin->LL_BoneID(full_bd.name);

    Fmatrix full_local = full_bd.bind_transform; // target's own bone length/offset
    Fmatrix live_world_for_children = live_parent_world;
    bool live_valid_here = false;

    if (live_id != BI_NONE && live_parent_valid)
    {
        const Fmatrix& live_world = live_kin->LL_GetBoneInstance(live_id).mTransform;
        Fmatrix live_parent_inv;
        live_parent_inv.invert(live_parent_world);
        Fmatrix live_local;
        live_local.mul_43(live_parent_inv, live_world);
        // take the source's rotation/scale basis only — keep target's own translation
        full_local.i = live_local.i;
        full_local.j = live_local.j;
        full_local.k = live_local.k;
        live_world_for_children = live_world;
        live_valid_here = true;
    }

    CBoneInstance& full_bi = full_kin->LL_GetBoneInstance(full_id);
    full_bi.mTransform.mul_43(full_parent_world, full_local);
    full_bi.mRenderTransform.mul_43(full_bi.mTransform, full_kin->LL_GetData(full_id).m2b_transform);

    for (CBoneData* child : full_bd.children)
        retarget_bone_recursive(full_kin, live_kin, *child, full_bi.mTransform, live_world_for_children, live_valid_here);
}

// sub-space rendering - main procedure
void R_dsgraph_structure::build_subspace()
{
    ZoneScoped;

    marker++; // !!! critical here

    if (o.precise_portals && RImplementation.rmPortals)
    {
        // Check if camera is too near to some portal - if so force DualRender
        Fvector box_radius;
        box_radius.set(o.query_box_side, o.query_box_side, o.query_box_side);
        Sectors_xrc.box_query(CDB::OPT_FULL_TEST, RImplementation.rmPortals, o.view_pos, box_radius);
        for (size_t K = 0; K < Sectors_xrc.r_count(); K++)
        {
            CPortal* pPortal = Portals[RImplementation.rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
            pPortal->bDualRender = TRUE;
        }
    }

    if (o.is_main_pass && (o.sector_id == IRender_Sector::INVALID_SECTOR_ID))
    {
        if (g_pGameLevel)
            g_pGameLevel->pHUD->Render_Last(context_id);
        return;
    }

    // Traverse sector/portal structure
    PortalTraverser.traverse(Sectors[o.sector_id], o.view_frustum, o.view_pos, o.xform, o.portal_traverse_flags);

    // Determine visibility for static geometry hierarchy
#if 0
    static xr_vector<Task*> static_geo_tasks;
    static_geo_tasks.resize(PortalTraverser.r_sectors.size());
#endif

    if (psDeviceFlags.test(rsDrawStatic))
    {
        for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
        {
            CSector* sector = PortalTraverser.r_sectors[s_it];
            dxRender_Visual* root = sector->root();
            //VERIFY(root->getType() == MT_HIERRARHY);

            const auto &children = static_cast<FHierrarhyVisual*>(root)->children;

            for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
            {
#if 0
                const auto traverse_children = [&, this](const TaskRange<size_t>& range)
                {
                    for (size_t id = range.cbegin(); id != range.cend(); ++id)
                    {
                        const auto& view = sector->r_frustums[v_it];
                        add_static(children[id], view, view.getMask());
                    }
                };

                if (o.mt_calculate) // NOTE: this code doesn't work until visuals maps are separated by worker ID.
                {
                    static_geo_tasks[s_it] = &xr_parallel_for(TaskRange<size_t>(0, children.size()), false, traverse_children);
                }
                else
                {
                    traverse_children(TaskRange<size_t>(0, children.size()));
                }
#else
                const auto& view = sector->r_frustums[v_it];
                add_static(root, view, view.getMask());
#endif
            }
        }
    }

    const bool collect_dynamic_any = (o.spatial_types != 0) && psDeviceFlags.test(rsDrawDynamic);

    if (collect_dynamic_any)
    {
        // Traverse object database
        g_pGamePersistent->SpatialSpace.q_frustum(lstRenderables, o.spatial_traverse_flags, o.spatial_types, o.view_frustum);

        if (o.spatial_traverse_flags & ISpatial_DB::O_ORDERED) // this should be inside of query functions
        {
            // Exact sorting order (front-to-back)
            std::sort(lstRenderables.begin(), lstRenderables.end(), [&](ISpatial* s1, ISpatial* s2)
                {
                    const float d1 = s1->GetSpatialData().sphere.P.distance_to_sqr(o.view_pos);
                    const float d2 = s2->GetSpatialData().sphere.P.distance_to_sqr(o.view_pos);
                    return d1 < d2;
                });
        }

        u32 uID_LTRACK = 0xffffffff;
        if (o.is_main_pass) // temporary
        {
            if (o.phase == CRender::PHASE_NORMAL)
            {
                RImplementation.uLastLTRACK++;
                if (!lstRenderables.empty())
                    uID_LTRACK = RImplementation.uLastLTRACK % lstRenderables.size();

                // update light-vis for current entity / actor
                IGameObject* O = g_pGameLevel->CurrentViewEntity();
                if (O)
                {
                    CROS_impl* R = (CROS_impl*)O->ROS();
                    if (R)
                        R->update(O);
                }
            }
        }

        const bool collect_lights = o.spatial_types & STYPE_LIGHTSOURCE;

        // Determine visibility for dynamic part of scene
        for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
        {
            ISpatial* spatial = lstRenderables[o_it];
            if (o.is_main_pass)
            {
                const auto& entity_pos = spatial->spatial_sector_point();
                const auto sector_id = detect_sector(entity_pos);
                spatial->spatial_updatesector(sector_id);
            }
            const auto& data = spatial->GetSpatialData();
            const auto& [type, sphere, sector_id] = std::tuple(data.type, data.sphere, data.sector_id);
            if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
                continue; // disassociated from S/P structure
            auto* sector = Sectors[sector_id];

            if (collect_lights && (type & STYPE_LIGHTSOURCE))
            {
                // lightsource
                light* L = (light*)spatial->dcast_Light();
                VERIFY(L);
                float lod = L->get_LOD();
                if (lod > EPS_L)
                {
                    // TODO: check for HOM flag
                    vis_data& vis = L->get_homdata();
                    if (RImplementation.HOM.visible(vis))
                        RImplementation.Lights.add_light(L);
                }
                continue;
            }

            if (PortalTraverser.i_marker != sector->r_marker)
                continue; // inactive (untouched) sector
            for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
            {
                const CFrustum& view = sector->r_frustums[v_it];
                if (!view.testSphere_dirty(spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
                    continue;

                if (o.is_main_pass)
                {
                    if (type & STYPE_RENDERABLE)
                    {
                        // renderable
                        IRenderable* renderable = spatial->dcast_Renderable();
                        VERIFY(renderable);

                        // Occlusion
                        //	casting is faster then using getVis method
                        vis_data& v_orig = ((dxRender_Visual*)renderable->GetRenderData().visual)->vis;
                        vis_data v_copy = v_orig;
                        v_copy.box.xform(renderable->GetRenderData().xform);
                        BOOL bVisible = RImplementation.HOM.visible(v_copy);
                        memcpy(v_orig.marker, v_copy.marker, sizeof(v_copy.marker));
                        v_orig.accept_frame = v_copy.accept_frame;
                        v_orig.hom_frame = v_copy.hom_frame;
                        v_orig.hom_tested = v_copy.hom_tested;
                        if (!bVisible)
                            break; // exit loop on frustums

                        // update light-vis for selected entity
                        if (o_it == uID_LTRACK && renderable->renderable_ROS())
                        {
                            // track lighting environment
                            CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
                            T->update(renderable);
                        }

                        // Rendering
                        renderable->renderable_Render(context_id, renderable);
                    }
                    break; // exit loop on frustums
                }
                else
                {
                    // renderable
                    IRenderable* renderable = spatial->dcast_Renderable();
                    if (nullptr == renderable)
                        continue; // unknown, but renderable object (r1_glow???)

                    renderable->renderable_Render(context_id, nullptr);
                }
            }
        }

        if (g_pGameLevel)
        {
#if RENDER != R_R1
            // Actor Shadow (Sun + Light)
            if (o.phase == CRender::PHASE_SMAP && ps_r__common_flags.test(RFLAG_ACTOR_SHADOW))
            {
                do
                {
                    IGameObject* viewEntity = g_pGameLevel->CurrentViewEntity();
                    if (viewEntity == nullptr)
                        break;
                    const auto& entity_pos = viewEntity->spatial_sector_point();
                    viewEntity->spatial_updatesector(detect_sector(entity_pos));
                    const auto sector_id = viewEntity->GetSpatialData().sector_id;
                    if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
                        break; // disassociated from S/P structure
                    CSector* sector = Sectors[sector_id];
                    if (PortalTraverser.i_marker != sector->r_marker)
                        break; // inactive (untouched) sector
                    for (const CFrustum& view : sector->r_frustums)
                    {
                        if (!view.testSphere_dirty(
                            viewEntity->GetSpatialData().sphere.P, viewEntity->GetSpatialData().sphere.R))
                            continue;

                        // renderable
                        g_pGameLevel->pHUD->Render_First(context_id);
                    }
                } while (0);
            }

            // Actor Body shadow: the weapon-shadow injection above only draws the HUD
            // (weapon) model into the shadow map, not the actor's own body.
            //
            // Root-caused 2026-07-27: the actor's *own* visual while worn with an outfit
            // is NOT a full body — CCustomOutfit::ApplySkinModel swaps it to the outfit's
            // "actor_visual" (e.g. actors\legs\novice_outfit.ogf), a legs-only model with
            // no head/hand geometry at all (by design, so the first-person HUD hands are
            // used instead — confirmed wanted behavior for the color-pass draw below).
            // Bone-visibility toggling (two earlier attempts) could never fix this: the
            // geometry for those parts simply isn't in that model. Two earlier attempts
            // confirmed via runtime log that the head/hand bones were already visible.
            //
            // Fix: render a SEPARATE, independent instance of the full-detail creature
            // visual (actors\stalker_hero\stalker_hero_1.ogf — the same complete model
            // regular NPCs use) for the shadow only, with its bone transforms copied
            // bone-by-bone (by name) from the actor's own live/posed skeleton so the
            // shadow's stance matches the actor's real animation instead of a static pose.
            if (o.phase == CRender::PHASE_SMAP && ps_r__common_flags.test(RFLAG_ACTOR_BODY))
            {
                do
                {
                    IGameObject* viewEntity = g_pGameLevel->CurrentViewEntity();
                    if (viewEntity == nullptr)
                        break;
                    const auto& entity_pos = viewEntity->spatial_sector_point();
                    viewEntity->spatial_updatesector(detect_sector(entity_pos));
                    const auto sector_id = viewEntity->GetSpatialData().sector_id;
                    if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
                        break; // disassociated from S/P structure
                    CSector* sector = Sectors[sector_id];
                    if (PortalTraverser.i_marker != sector->r_marker)
                        break; // inactive (untouched) sector
                    IRenderable* renderable = viewEntity->dcast_Renderable();
                    if (renderable == nullptr)
                        break;

                    for (const CFrustum& view : sector->r_frustums)
                    {
                        if (!view.testSphere_dirty(
                            viewEntity->GetSpatialData().sphere.P, viewEntity->GetSpatialData().sphere.R))
                            continue;

                        // Base body mesh only, NOT renderable->renderable_Render() (which
                        // also shadow-casts attachments like the torch/headlamp). The
                        // headlamp attaches at the LIVE (legs-only) skeleton's real bone
                        // position, while the full-detail silhouette below is a separately
                        // retargeted approximation of the head — the two don't line up
                        // exactly, so the headlamp cast a visibly detached second shadow
                        // next to the head's (confirmed unwanted in-game 2026-07-27).
                        // Simplest fix: the headlamp doesn't need its own shadow at all.
                        if (viewEntity->Visual() != nullptr)
                            RImplementation.add_Visual(context_id, renderable, viewEntity->Visual(), viewEntity->XFORM());
                    }
                } while (0);

                // Full-detail body, posed to match, for a complete silhouette. Deliberately
                // a SEPARATE do-while from the block above, not gated by its portal-marker /
                // sector-frustum checks: the reported "reverts to legs-only while moving,
                // comes back when moving back" (2026-07-27) pointed at that gate flipping
                // near sector/frustum boundaries — not at the retargeted pose data itself,
                // which logged as valid throughout, including while broken. add_Visual is
                // documented as doing no culling of its own, so this doesn't need that gate;
                // it only inherited it from copy-pasting the pre-existing weapon-shadow
                // block above (which has its own reasons to need it — portal-clipped
                // rendering for the HUD weapon specifically).
                do
                {
                    IGameObject* viewEntity = g_pGameLevel->CurrentViewEntity();
                    if (viewEntity == nullptr)
                        break;
                    IRenderable* renderable = viewEntity->dcast_Renderable();
                    if (renderable == nullptr)
                        break;

                    // Load once (Instance_Duplicate under the hood — safe to call every
                    // frame, but no need to; a single independent instance we keep posing
                    // ourselves is enough since it's only ever rendered from here).
                    static IRenderVisual* s_full_body_visual = nullptr;
                    static bool s_full_body_load_attempted = false;
                    if (!s_full_body_load_attempted)
                    {
                        s_full_body_load_attempted = true;
                        s_full_body_visual = RImplementation.model_Create("actors\\stalker_hero\\stalker_hero_1");
                    }
                    if (s_full_body_visual == nullptr)
                        break;

                    IKinematics* full_kin = s_full_body_visual->dcast_PKinematics();
                    IKinematics* live_kin = viewEntity->Visual() ? viewEntity->Visual()->dcast_PKinematics() : nullptr;
                    if (full_kin != nullptr && live_kin != nullptr)
                    {
                        // Proper retargeting (see retarget_bone_recursive above) — two
                        // earlier attempts that copied world/render transforms directly
                        // worked at idle but warped/collapsed during animation because
                        // they silently carried the live model's bone lengths into the
                        // full model's differently-proportioned mesh (confirmed in-game
                        // 2026-07-27).
                        const u16 full_root = full_kin->LL_GetBoneRoot();
                        const u16 live_root = live_kin->LL_GetBoneRoot();
                        if (full_root != BI_NONE)
                        {
                            retarget_bone_recursive(full_kin, live_kin, full_kin->LL_GetData(full_root),
                                Fidentity, Fidentity, live_root != BI_NONE);
                        }
                    }
                    RImplementation.add_Visual(context_id, renderable, s_full_body_visual, viewEntity->XFORM());
                } while (0);
            }

            // Actor Body (Dead Air "Тело игрока" option): the actor is normally excluded
            // from the main color pass render list (see spatial DB traversal above — this
            // is the classic X-Ray behavior where the local player never sees their own
            // body), the same way it's excluded from PHASE_SMAP unless force-injected
            // above. Mirror that force-injection here for PHASE_NORMAL so the actor's own
            // visual (not just the HUD weapon model) actually gets drawn when enabled.
            if (o.phase == CRender::PHASE_NORMAL && o.is_main_pass && ps_r__common_flags.test(RFLAG_ACTOR_BODY))
            {
                do
                {
                    IGameObject* viewEntity = g_pGameLevel->CurrentViewEntity();
                    if (viewEntity == nullptr)
                        break;
                    const auto& entity_pos = viewEntity->spatial_sector_point();
                    viewEntity->spatial_updatesector(detect_sector(entity_pos));
                    const auto sector_id = viewEntity->GetSpatialData().sector_id;
                    if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
                        break; // disassociated from S/P structure
                    CSector* sector = Sectors[sector_id];
                    if (PortalTraverser.i_marker != sector->r_marker)
                        break; // inactive (untouched) sector
                    IRenderable* renderable = viewEntity->dcast_Renderable();
                    if (renderable == nullptr)
                        break;
                    for (const CFrustum& view : sector->r_frustums)
                    {
                        if (!view.testSphere_dirty(
                            viewEntity->GetSpatialData().sphere.P, viewEntity->GetSpatialData().sphere.R))
                            continue;

                        // Base body mesh only — NOT renderable->renderable_Render(), which
                        // also draws attached items (torch/headlamp, radio) via
                        // CAttachmentOwner. Those attachments still need to render for the
                        // shadow (see the PHASE_SMAP block above), but in the color pass
                        // they show up as a headlamp mesh floating at head height, since
                        // the worn outfit's body model has no head geometry to mount it on
                        // (confirmed unwanted in-game 2026-07-27). add_Visual bypasses
                        // CActor::renderable_Render's attachment call entirely.
                        // A camera-to-bone distance heuristic was tried here to suppress
                        // the body draw during ladder-climbing (which clips into the
                        // camera) but logged data proved it can't work: camera-to-
                        // neck/shoulder distance sits in the same ~0.15-0.5m range during
                        // completely normal standing/walking as it does while climbing,
                        // because the camera is a fixed rig offset from the head/neck —
                        // that distance barely changes with animation state at all.
                        // Reverted (2026-07-27); the ladder clip remains unfixed and needs
                        // a different approach (e.g. a real camera-space/near-clip test,
                        // or detecting the climb state directly via a new cross-DLL query
                        // — neither attempted yet).
                        if (viewEntity->Visual() != nullptr)
                            RImplementation.add_Visual(context_id, renderable, viewEntity->Visual(), viewEntity->XFORM());
                    }
                } while (0);
            }
#endif

            if (o.is_main_pass)
                g_pGameLevel->pHUD->Render_Last(context_id);
        }
    }

#if 0
    // wait for static geo collecting to be done.
    for (auto* task : static_geo_tasks)
    {
        if (task)
            TaskScheduler->Wait(*task);
    }
#endif
}
} // namespace xray::render::RENDER_NAMESPACE
