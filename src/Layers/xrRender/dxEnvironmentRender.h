#ifndef dxEnvironmentRender_included
#define dxEnvironmentRender_included
#pragma once

#include "Include/xrRender/EnvironmentRender.h"
#include "Blender.h"

class CBlender_skybox : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: combiner"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C)
    {
        C.r_Pass("sky2", "sky2", FALSE, TRUE, FALSE);
#if defined(USE_DX9)
        C.r_Sampler_clf("s_sky0", "$null");
        C.r_Sampler_clf("s_sky1", "$null");
        C.r_Sampler_rtf("s_tonemap", "$user$tonemap"); //. hack
#elif defined(USE_DX11)
        // C.r_Sampler_clf		("s_sky0",		"$null"			);
        // C.r_Sampler_clf		("s_sky1",		"$null"			);
        C.r_dx11Texture("s_sky0", "$null");
        C.r_dx11Texture("s_sky1", "$null");
        C.r_dx11Sampler("smp_rtlinear");
        // C.r_Sampler_rtf		("s_tonemap",	"$user$tonemap"	);	//. hack
        C.r_dx11Texture("s_tonemap", "$user$tonemap"); //. hack
        C.PassSET_ZB(FALSE, FALSE);
#elif defined(USE_OGL)
        C.r_Sampler_clf("s_sky0", "$null");
        C.r_Sampler_clf("s_sky1", "$null");
        C.r_Sampler_rtf("s_tonemap", "$user$tonemap"); //. hack
        C.PassSET_ZB(FALSE, FALSE);
#else
#   error No graphics API selected or enabled!
#endif
        C.r_End();
    }
};

class dxEnvDescriptorRender : public IEnvDescriptorRender
{
    friend class dxEnvDescriptorMixerRender;

public:
    virtual void OnDeviceCreate(CEnvDescriptor& owner);
    virtual void OnDeviceDestroy();

    virtual void Copy(IEnvDescriptorRender& _in);

private:
    ref_texture sky_texture;
    ref_texture sky_texture_env;
    ref_texture clouds_texture;
};

class dxEnvDescriptorMixerRender : public IEnvDescriptorMixerRender
{
public:
    virtual void Copy(IEnvDescriptorMixerRender& _in);

    virtual void Destroy();
    virtual void Clear();
    virtual void lerp(IEnvDescriptorRender* inA, IEnvDescriptorRender* inB);
    // private:
public:
    STextureList sky_r_textures;
    STextureList sky_r_textures_env;
    STextureList clouds_r_textures;
};

class dxEnvironmentRender : public IEnvironmentRender, public CDeviceResetNotifier
{
public:
    dxEnvironmentRender();
    virtual void Copy(IEnvironmentRender& _in);

    virtual void OnFrame(CEnvironment& env);
    virtual void OnLoad();
    virtual void OnUnload();
    virtual void RenderSky(CEnvironment& env);
    virtual void RenderClouds(CEnvironment& env);
    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();
    virtual void OnDeviceReset();
    virtual particles_systems::library_interface const& particles_systems_library();

private:
    CBlender_skybox m_b_skybox;

    ref_shader sh_2sky;
    ref_geom sh_2geom;
    u32 tonemap_tstage_2sky;

    ref_shader clouds_sh;
    ref_geom clouds_geom;
    u32 tonemap_tstage_clouds;

    ref_texture tonemap;
    ref_texture tsky0, tsky1;
    u32 tsky0_tstage;
    u32 tsky1_tstage;
    u32 tclouds0_tstage;
    u32 tclouds1_tstage;
};

#endif //	EnvironmentRender_included
