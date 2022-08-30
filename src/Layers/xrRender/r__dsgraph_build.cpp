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

void R_dsgraph_structure::r_dsgraph_insert_dynamic(IRenderable* root, dxRender_Visual* pVisual, Fmatrix& xform, Fvector& Center)
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

void R_dsgraph_structure::r_dsgraph_insert_static(dxRender_Visual* pVisual)
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
///  Anomaly mod  ///
// Static geometry optimization
#define O_S_L1_S_LOW    10.f // geometry 3d volume size
#define O_S_L1_D_LOW    150.f // distance, after which it is not rendered
#define O_S_L2_S_LOW    100.f
#define O_S_L2_D_LOW    200.f
#define O_S_L3_S_LOW    500.f
#define O_S_L3_D_LOW    250.f
#define O_S_L4_S_LOW    2500.f
#define O_S_L4_D_LOW    350.f
#define O_S_L5_S_LOW    7000.f
#define O_S_L5_D_LOW    400.f
#define O_S_L6_S_LOW    12000.f
#define O_S_L6_D_LOW    800.f
#define O_S_L7_S_LOW    18000.f
#define O_S_L7_D_LOW    1000.f
#define O_S_L8_S_LOW    25000.f
#define O_S_L8_D_LOW    1200.f
#define O_S_L9_S_LOW    40000.f
#define O_S_L9_D_LOW    1500.f
#define O_S_L10_S_LOW   60000.f
#define O_S_L10_D_LOW   1800.f
#define O_S_L11_S_LOW   100000.f
#define O_S_L11_D_LOW   2000.f
#define O_S_L12_S_LOW   150000.f
#define O_S_L12_D_LOW   2500.f

#define O_S_L1_S_MED    25.f
#define O_S_L1_D_MED    50.f
#define O_S_L2_S_MED    200.f
#define O_S_L2_D_MED    150.f
#define O_S_L3_S_MED    1000.f
#define O_S_L3_D_MED    200.f
#define O_S_L4_S_MED    2500.f
#define O_S_L4_D_MED    300.f
#define O_S_L5_S_MED    7000.f
#define O_S_L5_D_MED    400.f
#define O_S_L6_S_MED    15000.f
#define O_S_L6_D_MED    750.f
#define O_S_L7_S_MED    20000.f
#define O_S_L7_D_MED    900.f
#define O_S_L8_S_MED    35000.f
#define O_S_L8_D_MED    1100.f
#define O_S_L9_S_MED    55000.f
#define O_S_L9_D_MED    1250.f
#define O_S_L10_S_MED   75000.f
#define O_S_L10_D_MED   1500.f
#define O_S_L11_S_MED   120000.f
#define O_S_L11_D_MED   1750.f
#define O_S_L12_S_MED   200000.f
#define O_S_L12_D_MED   2000.f

#define O_S_L1_S_HII    50.f
#define O_S_L1_D_HII    50.f
#define O_S_L2_S_HII    400.f
#define O_S_L2_D_HII    150.f
#define O_S_L3_S_HII    1500.f
#define O_S_L3_D_HII    200.f
#define O_S_L4_S_HII    5000.f
#define O_S_L4_D_HII    300.f
#define O_S_L5_S_HII    20000.f
#define O_S_L5_D_HII    350.f
#define O_S_L6_S_HII    30000.f
#define O_S_L6_D_HII    700.f
#define O_S_L7_S_HII    45000.f
#define O_S_L7_D_HII    900.f
#define O_S_L8_S_HII    55000.f
#define O_S_L8_D_HII    1100.f
#define O_S_L9_S_HII    70000.f
#define O_S_L9_D_HII    1250.f
#define O_S_L10_S_HII   90000.f
#define O_S_L10_D_HII   1500.f
#define O_S_L11_S_HII   150000.f
#define O_S_L11_D_HII   1750.f
#define O_S_L12_S_HII   250000.f
#define O_S_L12_D_HII   2000.f

#define O_S_L1_S_ULT    50.f
#define O_S_L1_D_ULT    40.f
#define O_S_L2_S_ULT    500.f
#define O_S_L2_D_ULT    125.f
#define O_S_L3_S_ULT    1750.f
#define O_S_L3_D_ULT    175.f
#define O_S_L4_S_ULT    5250.f
#define O_S_L4_D_ULT    250.f
#define O_S_L5_S_ULT    25000.f
#define O_S_L5_D_ULT    300.f
#define O_S_L6_S_ULT    40000.f
#define O_S_L6_D_ULT    600.f
#define O_S_L7_S_ULT    50000.f
#define O_S_L7_D_ULT    800.f
#define O_S_L8_S_ULT    65000.f
#define O_S_L8_D_ULT    1000.f
#define O_S_L9_S_ULT    85000.f
#define O_S_L9_D_ULT    1200.f
#define O_S_L10_S_ULT   150000.f
#define O_S_L10_D_ULT   1500.f
#define O_S_L11_S_ULT   250000.f
#define O_S_L11_D_ULT   1750.f
#define O_S_L12_S_ULT   500000.f
#define O_S_L12_D_ULT   2000.f

// Dyn geometry optimization

#define O_D_L1_S_LOW    1.f // geometry 3d volume size
#define O_D_L1_D_LOW    80.f // distance, after which it is not rendered
#define O_D_L2_S_LOW    3.f
#define O_D_L2_D_LOW    150.f
#define O_D_L3_S_LOW    4000.f
#define O_D_L3_D_LOW    250.f
#define O_D_L4_S_LOW    10000.f
#define O_D_L4_D_LOW    500.f
#define O_D_L5_S_LOW    25000.f
#define O_D_L5_D_LOW    750.f

#define O_D_L1_S_MED    2.f
#define O_D_L1_D_MED    40.f
#define O_D_L2_S_MED    4.f
#define O_D_L2_D_MED    100.f
#define O_D_L3_S_MED    4000.f
#define O_D_L3_D_MED    200.f
#define O_D_L4_S_MED    10000.f
#define O_D_L4_D_MED    400.f
#define O_D_L5_S_MED    25000.f
#define O_D_L5_D_MED    600.f

#define O_D_L1_S_HII    5.0f
#define O_D_L1_D_HII    30.f
#define O_D_L2_S_HII    10.f
#define O_D_L2_D_HII    80.f
#define O_D_L3_S_HII    4000.f
#define O_D_L3_D_HII    150.f
#define O_D_L4_S_HII    10000.f
#define O_D_L4_D_HII    300.f
#define O_D_L5_S_HII    25000.f
#define O_D_L5_D_HII    500.f

#define O_D_L1_S_ULT    7.5f
#define O_D_L1_D_ULT    30.f
#define O_D_L2_S_ULT    15.f
#define O_D_L2_D_ULT    50.f
#define O_D_L3_S_ULT    4000.f
#define O_D_L3_D_ULT    110.f
#define O_D_L4_S_ULT    10000.f
#define O_D_L4_D_ULT    250.f
#define O_D_L5_S_ULT    25000.f
#define O_D_L5_D_ULT    400.f

Fvector4 o_optimize_static_l1_dist = { O_S_L1_D_LOW, O_S_L1_D_MED, O_S_L1_D_HII, O_S_L1_D_ULT };
Fvector4 o_optimize_static_l1_size = { O_S_L1_S_LOW, O_S_L1_S_MED, O_S_L1_S_HII, O_S_L1_S_ULT };
Fvector4 o_optimize_static_l2_dist = { O_S_L2_D_LOW, O_S_L2_D_MED, O_S_L2_D_HII, O_S_L2_D_ULT };
Fvector4 o_optimize_static_l2_size = { O_S_L2_S_LOW, O_S_L2_S_MED, O_S_L2_S_HII, O_S_L2_S_ULT };
Fvector4 o_optimize_static_l3_dist = { O_S_L3_D_LOW, O_S_L3_D_MED, O_S_L3_D_HII, O_S_L3_D_ULT };
Fvector4 o_optimize_static_l3_size = { O_S_L3_S_LOW, O_S_L3_S_MED, O_S_L3_S_HII, O_S_L3_S_ULT };
Fvector4 o_optimize_static_l4_dist = { O_S_L4_D_LOW, O_S_L4_D_MED, O_S_L4_D_HII, O_S_L4_D_ULT };
Fvector4 o_optimize_static_l4_size = { O_S_L4_S_LOW, O_S_L4_S_MED, O_S_L4_S_HII, O_S_L4_S_ULT };
Fvector4 o_optimize_static_l5_dist = { O_S_L5_D_LOW, O_S_L5_D_MED, O_S_L5_D_HII, O_S_L5_D_ULT };
Fvector4 o_optimize_static_l5_size = { O_S_L5_S_LOW, O_S_L5_S_MED, O_S_L5_S_HII, O_S_L5_S_ULT };
Fvector4 o_optimize_static_l6_dist = { O_S_L6_D_LOW, O_S_L6_D_MED, O_S_L6_D_HII, O_S_L6_D_ULT };
Fvector4 o_optimize_static_l6_size = { O_S_L6_S_LOW, O_S_L6_S_MED, O_S_L6_S_HII, O_S_L6_S_ULT };
Fvector4 o_optimize_static_l7_dist = { O_S_L7_D_LOW, O_S_L7_D_MED, O_S_L7_D_HII, O_S_L7_D_ULT };
Fvector4 o_optimize_static_l7_size = { O_S_L7_S_LOW, O_S_L7_S_MED, O_S_L7_S_HII, O_S_L7_S_ULT };
Fvector4 o_optimize_static_l8_dist = { O_S_L8_D_LOW, O_S_L8_D_MED, O_S_L8_D_HII, O_S_L8_D_ULT };
Fvector4 o_optimize_static_l8_size = { O_S_L8_S_LOW, O_S_L8_S_MED, O_S_L8_S_HII, O_S_L8_S_ULT };
Fvector4 o_optimize_static_l9_dist = { O_S_L9_D_LOW, O_S_L9_D_MED, O_S_L9_D_HII, O_S_L9_D_ULT };
Fvector4 o_optimize_static_l9_size = { O_S_L9_S_LOW, O_S_L9_S_MED, O_S_L9_S_HII, O_S_L9_S_ULT };
Fvector4 o_optimize_static_l10_dist = { O_S_L10_D_LOW, O_S_L10_D_MED, O_S_L10_D_HII, O_S_L10_D_ULT };
Fvector4 o_optimize_static_l10_size = { O_S_L10_S_LOW, O_S_L10_S_MED, O_S_L10_S_HII, O_S_L10_S_ULT };
Fvector4 o_optimize_static_l11_dist = { O_S_L11_D_LOW, O_S_L11_D_MED, O_S_L11_D_HII, O_S_L11_D_ULT };
Fvector4 o_optimize_static_l11_size = { O_S_L11_S_LOW, O_S_L11_S_MED, O_S_L11_S_HII, O_S_L11_S_ULT };
Fvector4 o_optimize_static_l12_dist = { O_S_L12_D_LOW, O_S_L12_D_MED, O_S_L12_D_HII, O_S_L12_D_ULT };
Fvector4 o_optimize_static_l12_size = { O_S_L12_S_LOW, O_S_L12_S_MED, O_S_L12_S_HII, O_S_L12_S_ULT };

Fvector4 o_optimize_dynamic_l1_dist = { O_D_L1_D_LOW, O_D_L1_D_MED, O_D_L1_D_HII, O_D_L1_D_ULT };
Fvector4 o_optimize_dynamic_l1_size = { O_D_L1_S_LOW, O_D_L1_S_MED, O_D_L1_S_HII, O_D_L1_S_ULT };
Fvector4 o_optimize_dynamic_l2_dist = { O_D_L2_D_LOW, O_D_L2_D_MED, O_D_L2_D_HII, O_D_L2_D_ULT };
Fvector4 o_optimize_dynamic_l2_size = { O_D_L2_S_LOW, O_D_L2_S_MED, O_D_L2_S_HII, O_D_L2_S_ULT };
Fvector4 o_optimize_dynamic_l3_dist = { O_D_L3_D_LOW, O_D_L3_D_MED, O_D_L3_D_HII, O_D_L3_D_ULT };
Fvector4 o_optimize_dynamic_l3_size = { O_D_L3_S_LOW, O_D_L3_S_MED, O_D_L3_S_HII, O_D_L3_S_ULT };
Fvector4 o_optimize_dynamic_l4_dist = { O_D_L4_D_LOW, O_D_L4_D_MED, O_D_L4_D_HII, O_D_L4_D_ULT };
Fvector4 o_optimize_dynamic_l4_size = { O_D_L4_S_LOW, O_D_L4_S_MED, O_D_L4_S_HII, O_D_L4_S_ULT };
Fvector4 o_optimize_dynamic_l5_dist = { O_D_L5_D_LOW, O_D_L5_D_MED, O_D_L5_D_HII, O_D_L5_D_ULT };
Fvector4 o_optimize_dynamic_l5_size = { O_D_L5_S_LOW, O_D_L5_S_MED, O_D_L5_S_HII, O_D_L5_S_ULT };

#define BASE_FOV 65.f

IC float GetDistFromCamera(const Fvector& from_position)
// Aproximate, adjusted by fov, distance from camera to position (For right work when looking though binoculars and scopes)
{
    float distance = Device.vCameraPosition.distance_to(from_position);
    float fov_K = BASE_FOV / Device.fFOV;
    float adjusted_distane = distance / fov_K;

    return adjusted_distane;
}

IC bool IsValuableToRender(dxRender_Visual* pVisual, bool isStatic, bool sm, Fmatrix& transform_matrix)
{
    if ((isStatic && opt_static >= 1) || (!isStatic && opt_dynamic >= 1))
    {
        float sphere_volume = pVisual->getVisData().sphere.volume();

        float adjusted_distane = 0;

        if (isStatic)
            adjusted_distane = GetDistFromCamera(pVisual->vis.sphere.P);
        else
        // dynamic geometry position needs to be transformed by transform matrix, to get world coordinates, dont forget ;)
        {
            Fvector pos;
            transform_matrix.transform_tiny(pos, pVisual->vis.sphere.P);

            adjusted_distane = GetDistFromCamera(pos);
        }

        if (sm && !!psDeviceFlags.test(rsOptShadowGeom)) // Highest cut off for shadow map
        {
            if (sphere_volume < 50000.f && adjusted_distane > 160)
                // don't need geometry behind the farest sun shadow cascade
                return false;

            if ((sphere_volume < o_optimize_static_l1_size.z) && (adjusted_distane > o_optimize_static_l1_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l2_size.z) && (adjusted_distane > o_optimize_static_l2_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l3_size.z) && (adjusted_distane > o_optimize_static_l3_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l4_size.z) && (adjusted_distane > o_optimize_static_l4_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l5_size.z) && (adjusted_distane > o_optimize_static_l5_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l6_size.z) && (adjusted_distane > o_optimize_static_l6_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l7_size.z) && (adjusted_distane > o_optimize_static_l7_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l8_size.z) && (adjusted_distane > o_optimize_static_l8_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l9_size.z) && (adjusted_distane > o_optimize_static_l9_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l10_size.z) && (adjusted_distane > o_optimize_static_l10_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l11_size.z) && (adjusted_distane > o_optimize_static_l11_dist.z))
                return false;
            else if ((sphere_volume < o_optimize_static_l12_size.z) && (adjusted_distane > o_optimize_static_l12_dist.z))
                return false;
        }

        if (isStatic)
        {
            if (opt_static == 2)
            {
                if ((sphere_volume < o_optimize_static_l1_size.y) && (adjusted_distane > o_optimize_static_l1_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l2_size.y) && (adjusted_distane > o_optimize_static_l2_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l3_size.y) && (adjusted_distane > o_optimize_static_l3_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l4_size.y) && (adjusted_distane > o_optimize_static_l4_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l5_size.y) && (adjusted_distane > o_optimize_static_l5_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l6_size.y) && (adjusted_distane > o_optimize_static_l6_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l7_size.y) && (adjusted_distane > o_optimize_static_l7_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l8_size.y) && (adjusted_distane > o_optimize_static_l8_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l9_size.y) && (adjusted_distane > o_optimize_static_l9_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l10_size.y) && (adjusted_distane > o_optimize_static_l10_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l11_size.y) && (adjusted_distane > o_optimize_static_l11_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_static_l12_size.y) && (adjusted_distane > o_optimize_static_l12_dist.y))
                    return false;
            }
            else if (opt_static == 3)
            {
                if ((sphere_volume < o_optimize_static_l1_size.z) && (adjusted_distane > o_optimize_static_l1_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l2_size.z) && (adjusted_distane > o_optimize_static_l2_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l3_size.z) && (adjusted_distane > o_optimize_static_l3_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l4_size.z) && (adjusted_distane > o_optimize_static_l4_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l5_size.z) && (adjusted_distane > o_optimize_static_l5_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l6_size.z) && (adjusted_distane > o_optimize_static_l6_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l7_size.z) && (adjusted_distane > o_optimize_static_l7_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l8_size.z) && (adjusted_distane > o_optimize_static_l8_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l9_size.z) && (adjusted_distane > o_optimize_static_l9_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l10_size.z) && (adjusted_distane > o_optimize_static_l10_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l11_size.z) && (adjusted_distane > o_optimize_static_l11_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_static_l12_size.z) && (adjusted_distane > o_optimize_static_l12_dist.z))
                    return false;
            }
            else if (opt_static == 4)
            {
                if ((sphere_volume < o_optimize_static_l1_size.w) && (adjusted_distane > o_optimize_static_l1_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l2_size.w) && (adjusted_distane > o_optimize_static_l2_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l3_size.w) && (adjusted_distane > o_optimize_static_l3_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l4_size.w) && (adjusted_distane > o_optimize_static_l4_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l5_size.w) && (adjusted_distane > o_optimize_static_l5_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l6_size.w) && (adjusted_distane > o_optimize_static_l6_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l7_size.w) && (adjusted_distane > o_optimize_static_l7_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l8_size.w) && (adjusted_distane > o_optimize_static_l8_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l9_size.w) && (adjusted_distane > o_optimize_static_l9_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l10_size.w) && (adjusted_distane > o_optimize_static_l10_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l11_size.w) && (adjusted_distane > o_optimize_static_l11_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_static_l12_size.w) && (adjusted_distane > o_optimize_static_l12_dist.w))
                    return false;
            }
            else
            {
                if ((sphere_volume < o_optimize_static_l1_size.x) && (adjusted_distane > o_optimize_static_l1_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l2_size.x) && (adjusted_distane > o_optimize_static_l2_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l3_size.x) && (adjusted_distane > o_optimize_static_l3_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l4_size.x) && (adjusted_distane > o_optimize_static_l4_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l5_size.x) && (adjusted_distane > o_optimize_static_l5_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l6_size.x) && (adjusted_distane > o_optimize_static_l6_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l7_size.x) && (adjusted_distane > o_optimize_static_l7_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l8_size.x) && (adjusted_distane > o_optimize_static_l8_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l9_size.x) && (adjusted_distane > o_optimize_static_l9_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l10_size.x) && (adjusted_distane > o_optimize_static_l10_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l11_size.x) && (adjusted_distane > o_optimize_static_l11_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_static_l12_size.x) && (adjusted_distane > o_optimize_static_l12_dist.x))
                    return false;
            }
        }
        else
        {
            if (opt_dynamic == 2)
            {
                if ((sphere_volume < o_optimize_dynamic_l1_size.y) && (adjusted_distane > o_optimize_dynamic_l1_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l2_size.y) && (adjusted_distane > o_optimize_dynamic_l2_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l3_size.y) && (adjusted_distane > o_optimize_dynamic_l3_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l4_size.y) && (adjusted_distane > o_optimize_dynamic_l4_dist.y))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l5_size.y) && (adjusted_distane > o_optimize_dynamic_l5_dist.y))
                    return false;
            }
            else if (opt_dynamic == 3)
            {
                if ((sphere_volume < o_optimize_dynamic_l1_size.z) && (adjusted_distane > o_optimize_dynamic_l1_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l2_size.z) && (adjusted_distane > o_optimize_dynamic_l2_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l3_size.z) && (adjusted_distane > o_optimize_dynamic_l3_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l4_size.z) && (adjusted_distane > o_optimize_dynamic_l4_dist.z))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l5_size.z) && (adjusted_distane > o_optimize_dynamic_l5_dist.z))
                    return false;
            }
            else if (opt_dynamic == 4)
            {
                if ((sphere_volume < o_optimize_dynamic_l1_size.w) && (adjusted_distane > o_optimize_dynamic_l1_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l2_size.w) && (adjusted_distane > o_optimize_dynamic_l2_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l3_size.w) && (adjusted_distane > o_optimize_dynamic_l3_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l4_size.w) && (adjusted_distane > o_optimize_dynamic_l4_dist.w))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l5_size.w) && (adjusted_distane > o_optimize_dynamic_l5_dist.w))
                    return false;
            }
            else
            {
                if ((sphere_volume < o_optimize_dynamic_l1_size.x) && (adjusted_distane > o_optimize_dynamic_l1_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l2_size.x) && (adjusted_distane > o_optimize_dynamic_l2_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l3_size.x) && (adjusted_distane > o_optimize_dynamic_l3_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l4_size.x) && (adjusted_distane > o_optimize_dynamic_l4_dist.x))
                    return false;
                else if ((sphere_volume < o_optimize_dynamic_l5_size.x) && (adjusted_distane > o_optimize_dynamic_l5_dist.x))
                    return false;
            }
        }
    }
    return true;
}
///  Anomaly mod end  ///

void R_dsgraph_structure::add_leafs_Dynamic(IRenderable* root, dxRender_Visual* pVisual, Fmatrix& xform, bool ignore)
{
    if (nullptr == pVisual)
        return;

    if (!ignore && !IsValuableToRender(pVisual, false, phase == 1, xform))
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
                add_leafs_Dynamic(root, I._effect, xform);
            for (auto& pit : I._children_related)
                add_leafs_Dynamic(root, pit, xform);
            for (auto& pit : I._children_free)
                add_leafs_Dynamic(root, pit, xform);
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

            add_leafs_Dynamic(root, i, xform);
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
            add_leafs_Dynamic(root, pV->m_lod, xform);
        }
        else
        {
            pV->CalculateBones(TRUE);
            pV->CalculateWallmarks(root ? root->renderable_HUD() : false); //. bug?
            for (auto& i : pV->children)
            {
                i->vis.obj_data = pV->getVisData().obj_data; // Наследники используют шейдерные данные от родительского визуала
                                                             // [use shader data from parent model, rather than it childrens]
                add_leafs_Dynamic(root, i, xform);
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
        r_dsgraph_insert_dynamic(root, pVisual, xform, Tpos);
    }
        return;
    }
}

void R_dsgraph_structure::add_leafs_Static(dxRender_Visual* pVisual, Fmatrix* pTransform)
{
    if (!RImplementation.HOM.visible(pVisual->vis))
        return;

    if (!pVisual->_ignore_optimization && !IsValuableToRender(pVisual, true, phase == 1, *pTransform))
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
            add_leafs_Static(i, pTransform);
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
            add_leafs_Static(i, pTransform);
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
                add_leafs_Static(i, pTransform);
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

/* Xottab_DUTY: this function is only called from add_Static,
 * but we need a matrix, which is nullptr at this point
BOOL R_dsgraph_structure::add_Dynamic(dxRender_Visual* pVisual, u32 planes) // normal processing
{
    Fmatrix* pTransform;
    if (!pVisual->_ignore_optimization && !IsValuableToRender(pVisual, false, phase == 1, *pTransform))
        return FALSE;

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

void R_dsgraph_structure::add_Static(dxRender_Visual* pVisual, const CFrustum& view, u32 planes, Fmatrix* pTransform)
{
    if (!pVisual->_ignore_optimization && !IsValuableToRender(pVisual, true, phase == 1, *pTransform))
        return;

    vis_data& vis = pVisual->vis;

    // Check frustum visibility and calculate distance to visual's center
    EFC_Visible VIS = view.testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
    if (fcvNone == VIS)
        return;

    if (!RImplementation.HOM.visible(vis))
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
                add_Static(i, view, planes, pTransform);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_Static(i, pTransform);
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
                add_Static(i, view, planes, pTransform);
        }
        else
        {
            for (auto& i : pV->children)
                add_leafs_Static(i, pTransform);
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
                add_leafs_Static(i, pTransform);
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
