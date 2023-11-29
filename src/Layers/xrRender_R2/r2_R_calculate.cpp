#include "stdafx.h"

#include "xrEngine/CustomHUD.h"
#include "xrCore/Threading/TaskManager.hpp"

float g_fSCREEN;

extern float r_dtex_range;
extern float r_ssaDISCARD;
extern float r_ssaDONTSORT;
extern float r_ssaLOD_A;
extern float r_ssaLOD_B;
extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

extern int ps_r2_mt_calculate;
extern int ps_r2_mt_render;


//-----
void render_main::init()
{
    o.mt_calc_enabled = RImplementation.o.mt_calculate && !RImplementation.o.oldshadowcascades && !ps_r2_ls_flags.test(R2FLAG_ZFILL);
    o.mt_draw_enabled = false; // always on imm context
    o.active = true; // always active
}

void render_main::calculate()
{
    auto& dsgraph_main = RImplementation.get_imm_context();

    dsgraph_main.o.phase = CRender::PHASE_NORMAL;
    dsgraph_main.r_pmask(true, true, true); // enable priority "0,1",+ capture wmarks
    if (RImplementation.r_sun.o.active && RImplementation.o.oldshadowcascades)
        dsgraph_main.set_Recorder(&RImplementation.main_coarse_structure); // this is a show-stopper. Can't be paralleled with sun
    else
        dsgraph_main.set_Recorder(nullptr);
    dsgraph_main.o.use_hom = true;
    dsgraph_main.o.is_main_pass = true;
    dsgraph_main.o.sector_id = RImplementation.last_sector_id;
    dsgraph_main.o.portal_traverse_flags =
        CPortalTraverser::VQ_HOM | CPortalTraverser::VQ_SSA | CPortalTraverser::VQ_FADE;
    dsgraph_main.o.spatial_traverse_flags = ISpatial_DB::O_ORDERED;
    dsgraph_main.o.spatial_types = STYPE_RENDERABLE | STYPE_LIGHTSOURCE;
    dsgraph_main.o.view_pos = Device.vCameraPosition;
    dsgraph_main.o.xform = Device.mFullTransform;
    dsgraph_main.o.view_frustum = RImplementation.ViewBase;
    dsgraph_main.o.query_box_side = VIEWPORT_NEAR + EPS_L;
    dsgraph_main.o.precise_portals = true;
    dsgraph_main.o.mt_calculate = o.mt_calc_enabled;

    dsgraph_main.build_subspace();
}

void render_main::render()
{
    // TODO
}

//-----

void CRender::Calculate()
{
    // Transfer to global space to avoid deep pointer access
    IRender_Target* T = getTarget();
    float fov_factor = _sqr(90.f / Device.fFOV);
    g_fSCREEN = float(T->get_width(RCache) * T->get_height(RCache)) * fov_factor * (EPS_S + ps_r__LOD);
    r_ssaDISCARD = _sqr(ps_r__ssaDISCARD) / g_fSCREEN;
    r_ssaDONTSORT = _sqr(ps_r__ssaDONTSORT / 3) / g_fSCREEN;
    r_ssaLOD_A = _sqr(ps_r2_ssaLOD_A / 3) / g_fSCREEN;
    r_ssaLOD_B = _sqr(ps_r2_ssaLOD_B / 3) / g_fSCREEN;
    r_ssaGLOD_start = _sqr(ps_r__GLOD_ssa_start / 3) / g_fSCREEN;
    r_ssaGLOD_end = _sqr(ps_r__GLOD_ssa_end / 3) / g_fSCREEN;
    r_ssaHZBvsTEX = _sqr(ps_r__ssaHZBvsTEX / 3) / g_fSCREEN;
    r_dtex_range = ps_r2_df_parallax_range * g_fSCREEN / (1024.f * 768.f);

    // Configure
    o.distortion    = o.distortion_enabled;
    o.mt_calculate  = ps_r2_mt_calculate > 0;
    o.mt_render     = ps_r2_mt_render > 0;

    if (m_bFirstFrameAfterReset)
        return;

    auto& dsgraph_main = get_imm_context();

    // Detect camera-sector
    if (!Device.vCameraDirectionSaved.similar(Device.vCameraPosition, EPS_L))
    {
        const auto sector_id = dsgraph_main.detect_sector(Device.vCameraPosition);
        if (sector_id != IRender_Sector::INVALID_SECTOR_ID)
        {
            if (sector_id != last_sector_id)
                g_pGamePersistent->OnSectorChanged(sector_id);

            last_sector_id = sector_id;
        }
    }

    //
    Lights.Update();

    // Check if we touch some light even trough portal
    static xr_vector<ISpatial*> spatial_lights;
    g_pGamePersistent->SpatialSpace.q_sphere(spatial_lights, 0, STYPE_LIGHTSOURCE, Device.vCameraPosition, EPS_L);
    for (auto spatial : spatial_lights)
    {
        const auto& entity_pos = spatial->spatial_sector_point();
        spatial->spatial_updatesector(dsgraph_main.detect_sector(entity_pos));
        const auto sector_id = spatial->GetSpatialData().sector_id;
        if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
            continue; // disassociated from S/P structure

        VERIFY(spatial->GetSpatialData().type & STYPE_LIGHTSOURCE);
        // lightsource
        light* L = (light*)spatial->dcast_Light();
        VERIFY(L);
        Lights.add_light(L);
    }


    // Frustum
    ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

    TaskScheduler->Wait(*ProcessHOMTask);

    r_main.init();
    if (o.oldshadowcascades)
        r_sun_old.init();
    else
        r_sun.init();
#if RENDER != R_R2
    r_rain.init();
#endif

    // Main calc
    BasicStats.Culling.Begin();
    {
        r_main.run();
    }
    BasicStats.Culling.End();

    // Rain calc
#if RENDER != R_R2
    r_rain.run();
#endif

    // Sun calc
    if (o.oldshadowcascades)
        r_sun_old.run();
    else
        r_sun.run();
}
