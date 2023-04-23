#include "stdafx.h"
#include "Layers/xrRender/FVisual.h"

float g_fSCREEN;

extern float r_dtex_range;
extern float r_ssaDISCARD;
extern float r_ssaDONTSORT;
extern float r_ssaLOD_A;
extern float r_ssaLOD_B;
extern float r_ssaHZBvsTEX;
extern float r_ssaGLOD_start, r_ssaGLOD_end;

void CRender::Calculate()
{
    // Transfer to global space to avoid deep pointer access
    IRender_Target* T = getTarget();
    float fov_factor = _sqr(90.f / Device.fFOV);
    g_fSCREEN = float(T->get_width() * T->get_height()) * fov_factor * (EPS_S + ps_r__LOD);
    r_ssaDISCARD = _sqr(ps_r__ssaDISCARD) / g_fSCREEN;
    r_ssaDONTSORT = _sqr(ps_r__ssaDONTSORT / 3) / g_fSCREEN;
    r_ssaLOD_A = _sqr(ps_r2_ssaLOD_A / 3) / g_fSCREEN;
    r_ssaLOD_B = _sqr(ps_r2_ssaLOD_B / 3) / g_fSCREEN;
    r_ssaGLOD_start = _sqr(ps_r__GLOD_ssa_start / 3) / g_fSCREEN;
    r_ssaGLOD_end = _sqr(ps_r__GLOD_ssa_end / 3) / g_fSCREEN;
    r_ssaHZBvsTEX = _sqr(ps_r__ssaHZBvsTEX / 3) / g_fSCREEN;
    r_dtex_range = ps_r2_df_parallax_range * g_fSCREEN / (1024.f * 768.f);

    auto& dsgraph = RImplementation.alloc_context(eRDSG_MAIN);

    // Detect camera-sector
    if (!Device.vCameraDirectionSaved.similar(Device.vCameraPosition, EPS_L))
    {
        const auto sector_id = dsgraph.detect_sector(Device.vCameraPosition);
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
    g_SpatialSpace->q_sphere(spatial_lights, 0, STYPE_LIGHTSOURCE, Device.vCameraPosition, EPS_L);
    for (auto spatial : spatial_lights)
    {
        const auto& entity_pos = spatial->spatial_sector_point();
        spatial->spatial_updatesector(dsgraph.detect_sector(entity_pos));
        const auto sector_id = spatial->GetSpatialData().sector_id;
        if (sector_id == IRender_Sector::INVALID_SECTOR_ID)
            continue; // disassociated from S/P structure

        VERIFY(spatial->GetSpatialData().type & STYPE_LIGHTSOURCE);
        // lightsource
        light* L = (light*)spatial->dcast_Light();
        VERIFY(L);
        Lights.add_light(L);
    }

    // TODO: dsgraph setup goes here
}
