#ifndef dxEnvironmentRender_included
#define dxEnvironmentRender_included
#pragma once

#include "Include/xrRender/EnvironmentRender.h"

class dxEnvDescriptorRender : public IEnvDescriptorRender
{
    friend class dxEnvironmentRender;

public:
    virtual void OnDeviceCreate(CEnvDescriptor& owner);
    virtual void OnDeviceDestroy();

    virtual void Copy(IEnvDescriptorRender& _in);

private:
    ref_texture sky_texture;
    ref_texture sky_texture_env;
    ref_texture clouds_texture;
};

class dxEnvironmentRender : public IEnvironmentRender
{
public:
    dxEnvironmentRender();
    virtual void Copy(IEnvironmentRender& _in);

    virtual void RenderSky(CEnvironment& env);
    virtual void RenderClouds(CEnvironment& env);
    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();
    virtual void Clear();
    virtual void lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB);
    virtual particles_systems::library_interface const& particles_systems_library();

private:
    STextureList sky_r_textures;
    STextureList clouds_r_textures;

    ref_shader sh_2sky;
    ref_geom sh_2geom;

    ref_shader clouds_sh;
    ref_geom clouds_geom;

    u32 tsky0_tstage{};
    u32 tsky1_tstage{};
    u32 tclouds0_tstage{};
    u32 tclouds1_tstage{};

    ref_texture tsky0, tsky1;
    ref_texture t_envmap_0, t_envmap_1;

    ref_texture tonemap;
    u32 tonemap_tstage_2sky{ u32(-1) };
    u32 tonemap_tstage_clouds{ u32(-1) };
};

#endif //	EnvironmentRender_included
