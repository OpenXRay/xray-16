#include "stdafx.h"

#include "FHierrarhyVisual.h"
#include "SkeletonCustom.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/IRenderable.h"
#include "xrEngine/xr_object.h"

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

void R_dsgraph_structure::insert_dynamic(IRenderable* root, dxRender_Visual* pVisual, Fmatrix& xform, Fvector& Center)
{
    CRender& RI = RImplementation;

    if (pVisual->vis.marker == marker)
        return;
    pVisual->vis.marker = marker;

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
    ShaderElement* sh_d = pVisual->shader->E[4]._get(); // 4=L_special
    if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
    {
        mapDistort.insert_anyway(distSQ, _MatrixItemS({ SSA, root, pVisual, xform, sh_d })); // sh_d -> L_special
    }

    // Select shader
    ShaderElement* sh = RImplementation.rimp_select_sh_dynamic(pVisual, distSQ);
    if (nullptr == sh)
        return;
    if (!pmask[sh->flags.iPriority / 2])
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
        if (sh->flags.bEmissive)
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
    if (sh->flags.bWmark && pmask_wmark)
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
    CRender& RI = RImplementation;

    if (pVisual->vis.marker == marker)
        return;
    pVisual->vis.marker = marker;

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
    ShaderElement* sh_d = pVisual->shader->E[4]._get(); // 4=L_special
    if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority / 2])
    {
        mapDistort.insert_anyway(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh_d })); // sh_d -> L_special
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
        mapSorted.insert_anyway(distSQ, _MatrixItemS({ /*0*/SSA, nullptr, pVisual, Fidentity, sh }));
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
        mapEmissive.insert_anyway(distSQ, _MatrixItemS({ SSA, nullptr, pVisual, Fidentity, sh_d })); // sh_d -> L_special
    }
    if (sh->flags.bWmark && pmask_wmark)
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
            i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
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
            pV->CalculateWallmarks(root ? root->renderable_HUD() : false); //. bug?
            for (auto& i : pV->children)
            {
                i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
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
    if (use_hom && !RImplementation.HOM.visible(pVisual->vis))
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
            i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
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
            i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
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
        if (ssa > r_ssaLOD_B || phase == CRender::PHASE_SMAP)
#else
        if (ssa > r_ssaLOD_B)
#endif
        {
            // Add all children, doesn't perform any tests
            for (auto& i : pV->children)
            {
                i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
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
    vis_data& vis = pVisual->vis;

    // Check frustum visibility and calculate distance to visual's center
    EFC_Visible VIS = view.testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
    if (fcvNone == VIS)
        return;

    if (use_hom && !RImplementation.HOM.visible(vis))
        return;

    // If we get here visual is visible or partially visible
    switch (pVisual->Type)
    {
    case MT_PARTICLE_GROUP:
    {
        // Xottab_DUTY: for dynamic objects we need matrixб
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
        if (ssa > r_ssaLOD_B || phase == CRender::PHASE_SMAP)
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
    const auto portals_count = portals_data.size();
    const auto sectors_count = sectors_data.size();

    Sectors.resize(sectors_count);
    Portals.resize(portals_count);

    for (int idx = 0; idx < portals_count; ++idx)
    {
        auto* portal = xr_new<CPortal>();
        Portals[idx] = portal;
    }

    for (int idx = 0; idx < sectors_count; ++idx)
    {
        auto* sector = xr_new<CSector>();

        sector->unique_id = static_cast<IRender_Sector::sector_id_t>(idx);
        sector->setup(sectors_data[idx], Portals);
        Sectors[idx] = sector;
    }

    for (int idx = 0; idx < portals_count; ++idx)
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


//////////////////////////////////////////////////////////////////////////
// sub-space rendering - shortcut to render with frustum extracted from matrix
void R_dsgraph_structure::build_subspace(
    IRender_Sector::sector_id_t sector_id, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
    CFrustum temp;
    temp.CreateFromMatrix(mCombined, FRUSTUM_P_ALL & (~FRUSTUM_P_NEAR));
    build_subspace(sector_id, &temp, mCombined, _cop, _dynamic, _precise_portals);
}

// sub-space rendering - main procedure
void R_dsgraph_structure::build_subspace(IRender_Sector::sector_id_t sector_id, CFrustum* _frustum, Fmatrix& mCombined,
    Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
    VERIFY(sector_id != IRender_Sector::INVALID_SECTOR_ID);
    auto* _sector = Sectors[sector_id];

    PIX_EVENT(r_dsgraph_render_subspace);
    marker++; // !!! critical here

    if (_precise_portals && RImplementation.rmPortals)
    {
        // Check if camera is too near to some portal - if so force DualRender
        Fvector box_radius;
        box_radius.set(EPS_L * 20, EPS_L * 20, EPS_L * 20);
        Sectors_xrc.box_query(CDB::OPT_FULL_TEST, RImplementation.rmPortals, _cop, box_radius);
        for (int K = 0; K < Sectors_xrc.r_count(); K++)
        {
            CPortal* pPortal = Portals[RImplementation.rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
            pPortal->bDualRender = TRUE;
        }
    }

    // Traverse sector/portal structure
    PortalTraverser.traverse(_sector, *_frustum, _cop, mCombined, 0);

    // Determine visibility for static geometry hierrarhy
    if (psDeviceFlags.test(rsDrawStatic))
    {
        for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
        {
            CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
            dxRender_Visual* root = sector->root();
            for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
            {
                const auto& view = sector->r_frustums[v_it];
                add_static(root, view, view.getMask());
            }
        }
    }

    if (_dynamic && psDeviceFlags.test(rsDrawDynamic))
    {
        // Traverse object database
        g_SpatialSpace->q_frustum(lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE, *_frustum);

        // Determine visibility for dynamic part of scene
        for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
        {
            ISpatial* spatial = lstRenderables[o_it];
            const auto sector_id = spatial->GetSpatialData().sector_id;
            if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
                continue; // disassociated from S/P structure
            auto* sector = Sectors[sector_id];
            if (PortalTraverser.i_marker != sector->r_marker)
                continue; // inactive (untouched) sector
            for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
            {
                const CFrustum& view = sector->r_frustums[v_it];
                if (!view.testSphere_dirty(spatial->GetSpatialData().sphere.P, spatial->GetSpatialData().sphere.R))
                    continue;

                // renderable
                IRenderable* renderable = spatial->dcast_Renderable();
                if (nullptr == renderable)
                    continue; // unknown, but renderable object (r1_glow???)

                renderable->renderable_Render(nullptr);
            }
        }
#if RENDER != R_R1
        // Actor Shadow (Sun + Light)
        if (g_pGameLevel && phase == RImplementation.PHASE_SMAP && ps_r__common_flags.test(RFLAG_ACTOR_SHADOW))
        {
            do
            {
                IGameObject* viewEntity = g_pGameLevel->CurrentViewEntity();
                if (viewEntity == nullptr)
                    break;
                viewEntity->spatial_updatesector();
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
                    g_hud->Render_First();
                }
            } while (0);
        }
#endif
    }
}

void R_dsgraph_structure::build_R1_box(IRender_Sector::sector_id_t sector_id, Fbox& BB, int sh)
{
    VERIFY(sector_id != IRender_Sector::INVALID_SECTOR_ID);
    auto* S = Sectors[sector_id];

    PIX_EVENT(r_dsgraph_render_R1_box);

    lstVisuals.clear();
    lstVisuals.push_back(((CSector*)S)->root());

    for (size_t test = 0; test < lstVisuals.size(); ++test)
    {
        dxRender_Visual* V = lstVisuals[test];

        // Visual is 100% visible - simply add it
        switch (V->Type)
        {
        case MT_HIERRARHY: {
            // Add all children
            FHierrarhyVisual* pV = (FHierrarhyVisual*)V;
            for (auto& i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        case MT_SKELETON_ANIM:
        case MT_SKELETON_RIGID: {
            // Add all children	(s)
            CKinematics* pV = (CKinematics*)V;
            pV->CalculateBones(TRUE);
            for (auto& i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        case MT_LOD: {
            FLOD* pV = (FLOD*)V;
            for (auto& i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        default: {
            // Renderable visual
            ShaderElement* E2 = V->shader->E[sh]._get();
            if (E2 && !(E2->flags.bDistort))
            {
                for (u32 pass = 0; pass < E2->passes.size(); pass++)
                {
                    RCache.set_Element(E2, pass);
                    V->Render(-1.f);
                }
            }
        }
        break;
        }
    }
}
