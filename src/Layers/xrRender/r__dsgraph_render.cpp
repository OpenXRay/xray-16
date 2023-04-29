#include "stdafx.h"

#include "xrEngine/IRenderable.h"
#include "xrEngine/CustomHUD.h"

#include "FBasicVisual.h"

using namespace R_dsgraph;

extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

ICF float calcLOD(float ssa /*fDistSq*/, float /*R*/)
{
    return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

template <class T>
bool cmp_ssa(const T &lhs, const T &rhs)
{
    return lhs.ssa > rhs.ssa;
}

// Sorting by SSA and changes minimizations
template <typename T>
bool cmp_pass(const T& left, const T& right)
{
    if (left->first->equal(*right->first))
        return false;
    return left->second.ssa >= right->second.ssa;
}

void R_dsgraph_structure::render_graph(u32 _priority)
{
    PIX_EVENT(r_dsgraph_render_graph);
    RImplementation.BasicStats.Primitives.Begin(); // XXX: Refactor a bit later

    RCache.set_xform_world(Fidentity);

    // **************************************************** NORMAL
    // Perform sorting based on ScreenSpaceArea
    // Sorting by SSA and changes minimizations
    // Render several passes
    for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
    {
        auto& map = mapNormalPasses[_priority][iPass];

        map.get_any_p(nrmPasses);
        std::sort(nrmPasses.begin(), nrmPasses.end(), cmp_pass<mapNormal_T::value_type*>);
        for (const auto& it : nrmPasses)
        {
            RCache.set_Pass(it->first);
            RImplementation.apply_lmaterial();

            mapNormalItems& items = it->second;
            items.ssa = 0;

            std::sort(items.begin(), items.end(), cmp_ssa<_NormalItem>);
            for (const auto& item : items)
            {
                const float LOD = calcLOD(item.ssa, item.pVisual->vis.sphere.R);
#ifdef USE_DX11
                RCache.LOD.set_LOD(LOD);
#endif
                // --#SM+#-- Обновляем шейдерные данные модели [update shader values for this model]
                // RCache.hemi.c_update(item.pVisual);

                item.pVisual->Render(LOD, o.phase == CRender::PHASE_SMAP);
            }
            items.clear();

        }
        nrmPasses.clear();
        map.clear();
    }

    // **************************************************** MATRIX
    // Perform sorting based on ScreenSpaceArea
    // Sorting by SSA and changes minimizations
    // Render several passes
    for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
    {
        auto& map = mapMatrixPasses[_priority][iPass];

        map.get_any_p(matPasses);
        std::sort(matPasses.begin(), matPasses.end(), cmp_pass<mapMatrix_T::value_type*>);
        for (const auto& it : matPasses)
        {
            RCache.set_Pass(it->first);

            mapMatrixItems& items = it->second;
            items.ssa = 0;

            std::sort(items.begin(), items.end(), cmp_ssa<_MatrixItem>);
            for (auto& item : items)
            {
                RCache.set_xform_world(item.Matrix);
                RImplementation.apply_object(item.pObject);
                RImplementation.apply_lmaterial();

                const float LOD = calcLOD(item.ssa, item.pVisual->vis.sphere.R);
#ifdef USE_DX11
                RCache.LOD.set_LOD(LOD);
#endif
                // --#SM+#-- Обновляем шейдерные данные модели [update shader values for this model]
                // RCache.hemi.c_update(item.pVisual);

                item.pVisual->Render(LOD, o.phase == CRender::PHASE_SMAP);
            }
            items.clear();
        }
        matPasses.clear();
        map.clear();
    }

    RImplementation.BasicStats.Primitives.End(); // XXX: Refactor a bit later
}

//////////////////////////////////////////////////////////////////////////
// Helper classes and functions

/*
Предназначен для установки режима отрисовки HUD и возврата оригинального после отрисовки.
*/
class hud_transform_helper
{
    Fmatrix Pold;
    Fmatrix FTold;
    static u32 cullMode;
    static bool isActive;

public:
    hud_transform_helper()
    {
        extern ENGINE_API float psHUD_FOV;

        // Change projection
        Pold  = Device.mProject;
        FTold = Device.mFullTransform;

        // XXX: Xottab_DUTY: custom FOV. Implement it someday
        // It should be something like this:
        // float customFOV;
        // if (isCustomFOV)
        //     customFOV = V->getVisData().obj_data->m_hud_custom_fov;
        // else
        //     customFOV = psHUD_FOV * Device.fFOV;
        //
        // Device.mProject.build_projection(deg2rad(customFOV), Device.fASPECT,
        //    VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv.far_plane);
        //
        // Look at the function:
        // void __fastcall sorted_L1_HUD(mapSorted_Node* N)
        // In the commit:
        // https://github.com/ShokerStlk/xray-16-SWM/commit/869de0b6e74ac05990f541e006894b6fe78bd2a5#diff-4199ef700b18ce4da0e2b45dee1924d0R83

        Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV /* *Device.fASPECT*/), Device.fASPECT,
            VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv.far_plane);

        Device.mFullTransform.mul(Device.mProject, Device.mView);
        RCache.set_xform_project(Device.mProject);

        RImplementation.rmNear();

        // preserve culling mode
        cullMode = RCache.get_CullMode();
        isActive = true;
    }

    ~hud_transform_helper()
    {
        RImplementation.rmNormal();

        // Restore projection
        Device.mProject = Pold;
        Device.mFullTransform = FTold;
        RCache.set_xform_project(Device.mProject);
        // restore culling mode
        RCache.set_CullMode(cullMode);
        isActive = false;
    }

    static void apply_custom_state()
    {
        if (!isActive || !psHUD_Flags.test(HUD_LEFT_HANDED))
            return;

        // Change culling mode if HUD meshes were flipped
        if (cullMode != CULL_NONE)
        {
            RCache.set_CullMode(cullMode == CULL_CW ? CULL_CCW : CULL_CW);
        }
    }
};

u32 hud_transform_helper::cullMode = CULL_NONE;
bool hud_transform_helper::isActive = false;

template<class T>
void __fastcall render_item(u32 context_id, const T& item)
{
    auto& dsgraph = RImplementation.get_context(context_id);

    dxRender_Visual* V = item.second.pVisual;
    VERIFY(V && V->shader._get());
    RCache.set_Element(item.second.se);
    RCache.set_xform_world(item.second.Matrix);
    RImplementation.apply_object(item.second.pObject);
    RImplementation.apply_lmaterial();
    hud_transform_helper::apply_custom_state();
    //--#SM+#-- Обновляем шейдерные данные модели [update shader values for this model]
    //RCache.hemi.c_update(V);
    V->Render(calcLOD(item.first, V->vis.sphere.R), dsgraph.o.phase == CRender::PHASE_SMAP);
}

template<class T>
ICF void sort_front_to_back_render_and_clean(u32 context_id, T& vec)
{
    vec.traverse_left_right(context_id, render_item);
    vec.clear();
}

template<class T>
ICF void sort_back_to_front_render_and_clean(u32 context_id, T& vec)
{
    vec.traverse_right_left(context_id, render_item);
    vec.clear();
}

//////////////////////////////////////////////////////////////////////////
// HUD render
void R_dsgraph_structure::render_hud()
{
    PIX_EVENT(r_dsgraph_render_hud);

    if (!mapHUD.empty())
    {
        hud_transform_helper helper;
        sort_front_to_back_render_and_clean(context_id, mapHUD);
    }

#if RENDER == R_R1
    if (g_hud && g_hud->RenderActiveItemUIQuery())
        render_hud_ui(); // hud ui
#endif
}

void R_dsgraph_structure::render_hud_ui()
{
    VERIFY(g_hud && g_hud->RenderActiveItemUIQuery());

    PIX_EVENT(r_dsgraph_render_hud_ui);

    hud_transform_helper helper;

#if RENDER != R_R1
    // Targets, use accumulator for temporary storage
    const ref_rt rt_null;
    RCache.set_RT(0, 1);
    RCache.set_RT(0, 2);
    auto zb = RImplementation.Target->rt_Base_Depth;

#if (RENDER == R_R3) || (RENDER == R_R4) || (RENDER==R_GL)
    if (RImplementation.o.msaa)
        zb = RImplementation.Target->rt_MSAADepth;
#endif

    RImplementation.Target->u_setrt(
        RImplementation.o.albedo_wo ? RImplementation.Target->rt_Accumulator : RImplementation.Target->rt_Color,
        rt_null, rt_null, zb);
#endif // RENDER!=R_R1

    g_hud->RenderActiveItemUI();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void R_dsgraph_structure::render_sorted()
{
    PIX_EVENT(r_dsgraph_render_sorted);

    sort_back_to_front_render_and_clean(context_id, mapSorted);

    if (!mapHUDSorted.empty())
    {
        hud_transform_helper helper;
        sort_back_to_front_render_and_clean(context_id, mapHUDSorted);
    }
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void R_dsgraph_structure::render_emissive()
{
#if RENDER != R_R1
    PIX_EVENT(r_dsgraph_render_emissive);

    sort_front_to_back_render_and_clean(context_id, mapEmissive);

    if (!mapHUDEmissive.empty())
    {
        hud_transform_helper helper;
        sort_front_to_back_render_and_clean(context_id, mapHUDEmissive);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void R_dsgraph_structure::render_wmarks()
{
#if RENDER != R_R1
    PIX_EVENT(r_dsgraph_render_wmarks);

    sort_front_to_back_render_and_clean(context_id, mapWmark);
#endif
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void R_dsgraph_structure::render_distort()
{
    PIX_EVENT(r_dsgraph_render_distort);

    sort_back_to_front_render_and_clean(context_id, mapDistort);
}

#include "SkeletonCustom.h"
#include "FLOD.h"

void R_dsgraph_structure::render_R1_box(IRender_Sector::sector_id_t sector_id, Fbox& BB, int sh)
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
        case MT_HIERRARHY:
        {
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
        case MT_SKELETON_RIGID:
        {
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
        case MT_LOD:
        {
            FLOD* pV = (FLOD*)V;
            for (auto& i : pV->children)
            {
                dxRender_Visual* T = i;
                if (BB.intersect(T->vis.box))
                    lstVisuals.push_back(T);
            }
        }
        break;
        default:
        {
            // Renderable visual
            ShaderElement* E2 = V->shader->E[sh]._get();
            if (E2 && !(E2->flags.bDistort))
            {
                for (u32 pass = 0; pass < E2->passes.size(); pass++)
                {
                    RCache.set_Element(E2, pass);
                    V->Render(-1.f, o.phase == CRender::PHASE_SMAP);
                }
            }
        }
        break;
        }
    }
}
