#include "stdafx.h"

#include "Layers/xrRender_R2/r2.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/IRenderable.h"
#include "xrEngine/CustomHUD.h"

#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FLOD.h"

extern ENGINE_API float psHUD_FOV;

namespace xray::render::fg
{
using namespace R_dsgraph;

extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

ICF float calcLOD(float ssa , float )
{
    return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

template <class T>
bool cmp_ssa(const T &lhs, const T &rhs)
{
    return lhs.ssa > rhs.ssa;
}

template <typename T>
bool cmp_pass(const T& left, const T& right)
{
    if (left->first->equal(*right->first))
        return false;
    return left->second.ssa >= right->second.ssa;
}

void R_dsgraph_structure::render_graph(u32 _priority)
{
    PIX_EVENT_CTX(cmd_list, dsgraph_render_graph);
    RImplementation.BasicStats.Primitives.Begin();

    cmd_list.set_xform_world(Fidentity);

    {
        ZoneScopedN("dsgraph_render_static");
        PIX_EVENT_CTX(cmd_list, dsgraph_render_static);

        for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
        {
            auto& map = mapNormalPasses[_priority][iPass];

            map.get_any_p(nrmPasses);
            std::sort(nrmPasses.begin(), nrmPasses.end(), cmp_pass<mapNormal_T::value_type*>);
            for (const auto& it : nrmPasses)
            {
                cmd_list.set_Pass(it->first);
                cmd_list.apply_lmaterial();

                mapNormalItems& items = it->second;
                items.ssa = 0;

                std::sort(items.begin(), items.end(), cmp_ssa<_NormalItem>);
                for (const auto& item : items)
                {
                    const float LOD = calcLOD(item.ssa, item.pVisual->vis.sphere.R);
#ifdef USE_DX11
                    cmd_list.LOD.set_LOD(LOD);
#endif

                    item.pVisual->Render(cmd_list, LOD, o.phase == FrameGraphRenderer::PHASE_SMAP);
                }
                items.clear();

            }
            nrmPasses.clear();
            map.clear();
        }
    }

    {
        ZoneScopedN("dsgraph_render_dynamic");
        PIX_EVENT_CTX(cmd_list, dsgraph_render_dynamic);

        for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
        {
            auto& map = mapMatrixPasses[_priority][iPass];

            map.get_any_p(matPasses);
            std::sort(matPasses.begin(), matPasses.end(), cmp_pass<mapMatrix_T::value_type*>);
            for (const auto& it : matPasses)
            {
                cmd_list.set_Pass(it->first);

                mapMatrixItems& items = it->second;
                items.ssa = 0;

                std::sort(items.begin(), items.end(), cmp_ssa<_MatrixItem>);
                for (auto& item : items)
                {
                    cmd_list.set_xform_world(item.Matrix);
                    RImplementation.apply_object(cmd_list, item.pObject);
                    cmd_list.apply_lmaterial();

                    const float LOD = calcLOD(item.ssa, item.pVisual->vis.sphere.R);
#ifdef USE_DX11
                    cmd_list.LOD.set_LOD(LOD);
#endif

                    item.pVisual->Render(cmd_list, LOD, o.phase == FrameGraphRenderer::PHASE_SMAP);
                }
                items.clear();
            }
            matPasses.clear();
            map.clear();
        }
    }

    RImplementation.BasicStats.Primitives.End();
}

class hud_transform_helper
{
    Fmatrix Pold;
    static u32 cullMode;
    static bool isActive;

    CBackend& cmd_list;

public:
    explicit hud_transform_helper(CBackend& cmd_list_in)
        : cmd_list(cmd_list_in)
    {
        Pold  = Device.mProject;

        Fmatrix prj_new;
        prj_new.build_projection(deg2rad(psHUD_FOV * Device.fFOV ), Device.fASPECT,
            HUD_VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv.far_plane);
        cmd_list.set_xform_project(prj_new);

        RImplementation.rmNear(cmd_list);

        cullMode = cmd_list.get_CullMode();
        isActive = true;
    }

    ~hud_transform_helper()
    {
        RImplementation.rmNormal(cmd_list);

        cmd_list.set_xform_project(Pold);
        cmd_list.set_CullMode(cullMode);
        isActive = false;
    }

    static void apply_custom_state(CBackend& cmd_list)
    {
        if (!isActive || !psHUD_Flags.test(HUD_LEFT_HANDED))
            return;

        if (cullMode != CULL_NONE)
        {
            cmd_list.set_CullMode(cullMode == CULL_CW ? CULL_CCW : CULL_CW);
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
    dsgraph.cmd_list.set_Element(item.second.se);
    dsgraph.cmd_list.set_xform_world(item.second.Matrix);
    RImplementation.apply_object(dsgraph.cmd_list, item.second.pObject);
    dsgraph.cmd_list.apply_lmaterial();
    hud_transform_helper::apply_custom_state(dsgraph.cmd_list);
    V->Render(dsgraph.cmd_list, calcLOD(item.first, V->vis.sphere.R), dsgraph.o.phase == FrameGraphRenderer::PHASE_SMAP);
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

void R_dsgraph_structure::render_hud()
{
    ZoneScoped;
    PIX_EVENT_CTX(cmd_list, dsgraph_render_hud);

    if (!mapHUD.empty())
    {
        hud_transform_helper helper{ cmd_list };
        sort_front_to_back_render_and_clean(context_id, mapHUD);
    }

#if RENDER == R_R1
    if (g_pGameLevel->pHUD && g_pGameLevel->pHUD->RenderActiveItemUIQuery())
        render_hud_ui();
#endif
}

void R_dsgraph_structure::render_hud_ui()
{
    ZoneScoped;
    CCustomHUD* levelHud = g_pGameLevel->pHUD;
    VERIFY(levelHud && levelHud->RenderActiveItemUIQuery());

    PIX_EVENT_CTX(cmd_list, dsgraph_render_hud_ui);

    hud_transform_helper helper{ cmd_list };

#if RENDER != R_R1
    const ref_rt rt_null;
    cmd_list.set_RT(0, 1);
    cmd_list.set_RT(0, 2);
    auto zb = RImplementation.m_pTarget->rt_Base_Depth;

#if (RENDER == R_R3) || (RENDER == R_R4) || (RENDER==R_GL)
    if (RImplementation.o.msaa)
        zb = RImplementation.m_pTarget->rt_MSAADepth;
#endif

    RImplementation.m_pTarget->u_setrt(cmd_list,
        RImplementation.o.albedo_wo ? RImplementation.m_pTarget->rt_Accumulator : RImplementation.m_pTarget->rt_Color,
        rt_null, rt_null, zb);
#endif

    levelHud->RenderActiveItemUI();
}

void R_dsgraph_structure::render_sorted()
{
    ZoneScoped;
    PIX_EVENT_CTX(cmd_list, dsgraph_render_sorted);

    sort_back_to_front_render_and_clean(context_id, mapSorted);

    if (!mapHUDSorted.empty())
    {
        hud_transform_helper helper{ cmd_list };
        sort_back_to_front_render_and_clean(context_id, mapHUDSorted);
    }
}

void R_dsgraph_structure::render_emissive()
{
#if RENDER != R_R1
    ZoneScoped;
    PIX_EVENT_CTX(cmd_list, dsgraph_render_emissive);

    sort_front_to_back_render_and_clean(context_id, mapEmissive);

    if (!mapHUDEmissive.empty())
    {
        hud_transform_helper helper{ cmd_list };
        sort_front_to_back_render_and_clean(context_id, mapHUDEmissive);
    }
#endif
}

void R_dsgraph_structure::render_wmarks()
{
#if RENDER != R_R1
    ZoneScoped;
    PIX_EVENT(dsgraph_render_wmarks);

    sort_front_to_back_render_and_clean(context_id, mapWmark);
#endif
}

void R_dsgraph_structure::render_distort()
{
    ZoneScoped;
    PIX_EVENT(dsgraph_render_distort);

    sort_back_to_front_render_and_clean(context_id, mapDistort);
}

void R_dsgraph_structure::render_R1_box(IRender_Sector::sector_id_t sector_id, Fbox& BB, int sh)
{
    VERIFY(sector_id != IRender_Sector::INVALID_SECTOR_ID);
    auto* S = Sectors[sector_id];

    PIX_EVENT(dsgraph_render_R1_box);

    lstVisuals.clear();
    lstVisuals.push_back(((CSector*)S)->root());

    for (size_t test = 0; test < lstVisuals.size(); ++test)
    {
        dxRender_Visual* V = lstVisuals[test];

        switch (V->Type)
        {
        case MT_HIERRARHY:
        {
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
            ShaderElement* E2 = V->shader->E[sh]._get();
            if (E2 && !(E2->flags.bDistort))
            {
                for (u32 pass = 0; pass < E2->passes.size(); pass++)
                {
                    cmd_list.set_Element(E2, pass);
                    V->Render(cmd_list, -1.f, o.phase == FrameGraphRenderer::PHASE_SMAP);
                }
            }
        }
        break;
        }
    }
}
}
