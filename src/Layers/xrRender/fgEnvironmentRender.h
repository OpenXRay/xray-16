#pragma once

#include "Include/xrRender/EnvironmentRender.h"

namespace xray::render::fg
{
class FGEnvironmentRender;

class FGEnvDescriptorRender : public IEnvDescriptorRender
{
    friend class FGEnvironmentRender;

public:
    void OnDeviceCreate(CEnvDescriptor& owner) override;
    void OnDeviceDestroy() override;

    void Copy(IEnvDescriptorRender& _in) override;

private:
    ref_texture sky_texture;
    ref_texture sky_texture_env;
    ref_texture clouds_texture;
};

class FGEnvironmentRender : public IEnvironmentRender
{
public:
    FGEnvironmentRender();

    void Copy(IEnvironmentRender& _in) override;

    void RenderSky(CEnvironment& env) override {}

    void RenderClouds(CEnvironment& env) override {}

    void OnDeviceCreate() override;
    void OnDeviceDestroy() override;
    void Clear() override;
    void lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB) override;
    const particles_systems::library_interface& particles_systems_library() override;

private:
    using RuntimeTextureList = xr_vector<std::pair<u32, ref_texture>>;
    RuntimeTextureList sky_r_textures;
    RuntimeTextureList clouds_r_textures;

    ref_texture tsky0, tsky1;
    ref_texture t_envmap_0, t_envmap_1;
    ref_texture tonemap;

    u32 tsky0_tstage{};
    u32 tsky1_tstage{};
    u32 tclouds0_tstage{};
    u32 tclouds1_tstage{};
    u32 tonemap_tstage_2sky{ u32(-1) };
    u32 tonemap_tstage_clouds{ u32(-1) };
};
} // namespace xray::render::fg
