#include "stdafx.h"

#include "fgEnvironmentRender.h"

#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::fg
{
void FGEnvDescriptorRender::Copy(IEnvDescriptorRender& _in)
{
    *this = *static_cast<FGEnvDescriptorRender*>(&_in);
}

void FGEnvDescriptorRender::OnDeviceCreate(CEnvDescriptor& owner)
{
    if (owner.sky_texture_name.size())
        sky_texture.create(owner.sky_texture_name.c_str());
    if (owner.sky_texture_env_name.size())
        sky_texture_env.create(owner.sky_texture_env_name.c_str());
    if (owner.clouds_texture_name.size())
        clouds_texture.create(owner.clouds_texture_name.c_str());
}

void FGEnvDescriptorRender::OnDeviceDestroy()
{
    sky_texture.destroy();
    sky_texture_env.destroy();
    clouds_texture.destroy();
}

FGEnvironmentRender::FGEnvironmentRender()
{
    tsky0.create(r2_T_sky0);
    tsky1.create(r2_T_sky1);
    t_envmap_0.create(r2_T_envs0);
    t_envmap_1.create(r2_T_envs1);
    tonemap.create(r2_RT_luminance_cur);
}

void FGEnvironmentRender::Copy(IEnvironmentRender& _in)
{
    *this = *static_cast<FGEnvironmentRender*>(&_in);
}

const particles_systems::library_interface& FGEnvironmentRender::particles_systems_library()
{
    return RImplementation.m_PSLibrary;
}

void FGEnvironmentRender::Clear()
{
    std::pair<u32, ref_texture> zero = std::make_pair(u32(0), ref_texture(nullptr));
    sky_r_textures.clear();
    sky_r_textures.push_back(zero);
    sky_r_textures.push_back(zero);
    sky_r_textures.push_back(zero);

    clouds_r_textures.clear();
    clouds_r_textures.push_back(zero);
    clouds_r_textures.push_back(zero);
    clouds_r_textures.push_back(zero);
}

void FGEnvironmentRender::lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB)
{
    auto* pA = static_cast<FGEnvDescriptorRender*>(inA);
    auto* pB = static_cast<FGEnvDescriptorRender*>(inB);

    sky_r_textures.clear();
    sky_r_textures.emplace_back(tsky0_tstage, pA->sky_texture);
    sky_r_textures.emplace_back(tsky1_tstage, pB->sky_texture);
    if (tonemap_tstage_2sky != u32(-1))
        sky_r_textures.emplace_back(tonemap_tstage_2sky, tonemap);

    clouds_r_textures.clear();
    clouds_r_textures.emplace_back(tclouds0_tstage, pA->clouds_texture);
    clouds_r_textures.emplace_back(tclouds1_tstage, pB->clouds_texture);
    if (tonemap_tstage_clouds != u32(-1))
        clouds_r_textures.emplace_back(tonemap_tstage_clouds, tonemap);

    auto e0 = sky_r_textures[0].second->surface_get();
    auto e1 = sky_r_textures[1].second->surface_get();
    tsky0->surface_set(e0);
    _RELEASE(e0);
    tsky1->surface_set(e1);
    _RELEASE(e1);

    const bool menu_pp = g_pGamePersistent->OnRenderPPUI_query();
    e0 = menu_pp ? nullptr : pA->sky_texture_env->surface_get();
    e1 = menu_pp ? nullptr : pB->sky_texture_env->surface_get();
    t_envmap_0->surface_set(e0);
    _RELEASE(e0);
    t_envmap_1->surface_set(e1);
    _RELEASE(e1);
}

void FGEnvironmentRender::OnDeviceCreate()
{
    if (GEnv.isDedicatedServer)
        return;
}

void FGEnvironmentRender::OnDeviceDestroy()
{
    sky_r_textures.clear();
    clouds_r_textures.clear();

    tsky0->surface_set(nullptr);
    tsky1->surface_set(nullptr);
    t_envmap_0->surface_set(nullptr);
    t_envmap_1->surface_set(nullptr);
    tonemap->surface_set(nullptr);

    tsky0_tstage = 0;
    tsky1_tstage = 0;
    tclouds0_tstage = 0;
    tclouds1_tstage = 0;
    tonemap_tstage_2sky = u32(-1);
    tonemap_tstage_clouds = u32(-1);
}
} // namespace xray::render::fg
