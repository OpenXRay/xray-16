// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "xrCDB/xrXRC.h"
#include "xrScriptEngine/script_engine.hpp"

#include "xrCore/ModuleLookup.hpp"

extern xr_token* vid_quality_token;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void){};
CEngineAPI::CEngineAPI()
{
    hGame = 0;
    hRender = 0;
    hTuner = 0;
    pCreate = 0;
    pDestroy = 0;
    tune_pause = dummy;
    tune_resume = dummy;
}

CEngineAPI::~CEngineAPI()
{
    // destroy quality token here
    if (vid_quality_token)
    {
        xr_free(vid_quality_token);
        vid_quality_token = NULL;
    }
}

extern u32 renderer_value; // con cmd
ENGINE_API int g_current_renderer = 0;

bool is_enough_address_space_available()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

#ifndef DEDICATED_SERVER

void CEngineAPI::InitializeNotDedicated()
{
    constexpr pcstr r2_name = "xrRender_R2";
    constexpr pcstr r3_name = "xrRender_R3";
    constexpr pcstr r4_name = "xrRender_R4";

    if (psDeviceFlags.test(rsR4))
    {
        // try to initialize R4
        hRender->open(r4_name);
        if (!hRender->exist())
        {
            // try to load R1
            Log("! ...Failed - incompatible hardware/pre-Vista OS.");
            psDeviceFlags.set(rsR2, true);
        }
    }

    if (psDeviceFlags.test(rsR3))
    {
        // try to initialize R3
        hRender->open(r3_name);
        if (!hRender->exist())
        {
            // try to load R1
            Log("! ...Failed - incompatible hardware/pre-Vista OS.");
            psDeviceFlags.set(rsR2, true);
        }
        else
            g_current_renderer = 3;
    }

    if (psDeviceFlags.test(rsR2))
    {
        // try to initialize R2
        psDeviceFlags.set(rsR4, false);
        psDeviceFlags.set(rsR3, false);
        hRender->open(r2_name);
        if (!hRender->exist())
        {
            // try to load R1
            Log("! ...Failed - incompatible hardware.");
        }
        else
            g_current_renderer = 2;
    }
}
#endif // DEDICATED_SERVER

void CEngineAPI::Initialize(void)
{
    //////////////////////////////////////////////////////////////////////////
    // render
    constexpr pcstr r1_name = "xrRender_R1";

#ifndef DEDICATED_SERVER
    InitializeNotDedicated();
#endif // DEDICATED_SERVER

    if (!hRender->exist())
    {
        // try to load R1
        psDeviceFlags.set(rsR4, false);
        psDeviceFlags.set(rsR3, false);
        psDeviceFlags.set(rsR2, false);
        renderer_value = 0; // con cmd

        hRender->open(r1_name);
        if (!hRender->exist())
            R_CHK(GetLastError());
        R_ASSERT(hRender);
        g_current_renderer = 1;
    }
    // ask current renderer to setup GlobalEnv
    using SetupEnvFunc = void (*)();
    auto setupEnv = (SetupEnvFunc)hRender->getProcAddress("SetupEnv");
    R_ASSERT(setupEnv);
    setupEnv();
    // game
    {
        constexpr pcstr g_name = "xrGame";
        hGame = std::make_unique<XRay::Module>(g_name);

        if (!hGame->exist())
            R_CHK(GetLastError());
        R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

        pCreate = (Factory_Create*)hGame->getProcAddress("xrFactory_Create");
        R_ASSERT(pCreate);

        pDestroy = (Factory_Destroy*)hGame->getProcAddress("xrFactory_Destroy");
        R_ASSERT(pDestroy);
    }

    //////////////////////////////////////////////////////////////////////////
    // vTune
    tune_enabled = FALSE;
    if (strstr(Core.Params, "-tune"))
    {
        constexpr pcstr g_name = "vTuneAPI";
        hTuner = std::make_unique<XRay::Module>(g_name);

        if (!hTuner->exist())
            R_CHK(GetLastError());
        R_ASSERT2(hTuner, "Intel vTune is not installed");

        tune_enabled = true;

        tune_pause = (VTPause*)hTuner->getProcAddress("VTPause");
        R_ASSERT(tune_pause);

        tune_resume = (VTResume*)hTuner->getProcAddress("VTResume");
        R_ASSERT(tune_resume);
    }
}

void CEngineAPI::Destroy(void)
{
    pCreate = 0;
    pDestroy = 0;
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

extern "C" {
typedef bool __cdecl SupportsAdvancedRendering(void);
typedef bool _declspec(dllexport) SupportsDX10Rendering();
typedef bool _declspec(dllexport) SupportsDX11Rendering();
};

void CEngineAPI::CreateRendererList()
{
#ifdef DEDICATED_SERVER

    vid_quality_token = xr_alloc<xr_token>(2);

    vid_quality_token[0].id = 0;
    vid_quality_token[0].name = xr_strdup("renderer_r1");

    vid_quality_token[1].id = -1;
    vid_quality_token[1].name = NULL;

#else
    // TODO: ask renderers if they are supported!
    if (vid_quality_token != NULL)
        return;
    bool bSupports_r2 = false;
    bool bSupports_r2_5 = false;
    bool bSupports_r3 = false;
    bool bSupports_r4 = false;

    constexpr pcstr r2_name = "xrRender_R2";
    constexpr pcstr r3_name = "xrRender_R3";
    constexpr pcstr r4_name = "xrRender_R4";

    hRender = std::make_unique<XRay::Module>();

    if (strstr(Core.Params, "-perfhud_hack"))
    {
        bSupports_r2 = true;
        bSupports_r2_5 = true;
        bSupports_r3 = true;
        bSupports_r4 = true;
    }
    else
    {
        // try to initialize R2
        hRender->open(r2_name);
        if (hRender->exist())
        {
            bSupports_r2 = true;
            auto test_rendering =
                (SupportsAdvancedRendering*)hRender->getProcAddress("SupportsAdvancedRendering");
            R_ASSERT(test_rendering);
            bSupports_r2_5 = test_rendering();
        }

        // try to initialize R3
        Log("Loading DLL:", r3_name);
        // Hide "d3d10.dll not found" message box for XP
        SetErrorMode(SEM_FAILCRITICALERRORS);
        hRender->open(r3_name);
        // Restore error handling
        SetErrorMode(0);
        if (hRender->exist())
        {
            auto test_dx10_rendering =
                (SupportsDX10Rendering*)hRender->getProcAddress("SupportsDX10Rendering");
            R_ASSERT(test_dx10_rendering);
            bSupports_r3 = test_dx10_rendering();
        }

        // try to initialize R4
        Log("Loading DLL:", r4_name);
        // Hide "d3d10.dll not found" message box for XP
        SetErrorMode(SEM_FAILCRITICALERRORS);
        hRender->open(r4_name);
        // Restore error handling
        SetErrorMode(0);
        if (hRender->exist())
        {
            auto test_dx11_rendering =
                (SupportsDX11Rendering*)hRender->getProcAddress("SupportsDX11Rendering");
            R_ASSERT(test_dx11_rendering);
            bSupports_r4 = test_dx11_rendering();
        }
    }

    bool proceed = true;
    xr_vector<LPCSTR> tmp;
    tmp.push_back("renderer_r1");
    if (proceed &= bSupports_r2, proceed)
    {
        tmp.push_back("renderer_r2a");
        tmp.push_back("renderer_r2");
    }
    if (proceed &= bSupports_r2_5, proceed)
        tmp.push_back("renderer_r2.5");
    if (proceed &= bSupports_r3, proceed)
        tmp.push_back("renderer_r3");
    if (proceed &= bSupports_r4, proceed)
        tmp.push_back("renderer_r4");
    u32 _cnt = tmp.size() + 1;
    vid_quality_token = xr_alloc<xr_token>(_cnt);

    vid_quality_token[_cnt - 1].id = -1;
    vid_quality_token[_cnt - 1].name = NULL;

#ifdef DEBUG
    Msg("Available render modes[%d]:", tmp.size());
#endif // DEBUG
    for (u32 i = 0; i < tmp.size(); ++i)
    {
        vid_quality_token[i].id = i;
        vid_quality_token[i].name = tmp[i];
#ifdef DEBUG
        Msg("[%s]", tmp[i]);
#endif // DEBUG
    }

/*
if(vid_quality_token != NULL) return;

D3DCAPS9 caps;
CHW _HW;
_HW.CreateD3D ();
_HW.pD3D->GetDeviceCaps (D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&caps);
_HW.DestroyD3D ();
u16 ps_ver_major = u16 ( u32(u32(caps.PixelShaderVersion)&u32(0xf << 8ul))>>8 );

xr_vector<LPCSTR> _tmp;
u32 i = 0;
for(; i<5; ++i)
{
bool bBreakLoop = false;
switch (i)
{
case 3: //"renderer_r2.5"
if (ps_ver_major < 3)
bBreakLoop = true;
break;
case 4: //"renderer_r_dx10"
bBreakLoop = true;
break;
default: ;
}

if (bBreakLoop) break;

_tmp.push_back (NULL);
LPCSTR val = NULL;
switch (i)
{
case 0: val ="renderer_r1"; break;
case 1: val ="renderer_r2a"; break;
case 2: val ="renderer_r2"; break;
case 3: val ="renderer_r2.5"; break;
case 4: val ="renderer_r_dx10"; break; // -)
}
_tmp.back() = xr_strdup(val);
}
u32 _cnt = _tmp.size()+1;
vid_quality_token = xr_alloc<xr_token>(_cnt);

vid_quality_token[_cnt-1].id = -1;
vid_quality_token[_cnt-1].name = NULL;

#ifdef DEBUG
Msg("Available render modes[%d]:",_tmp.size());
#endif // DEBUG
for(u32 i=0; i<_tmp.size();++i)
{
vid_quality_token[i].id = i;
vid_quality_token[i].name = _tmp[i];
#ifdef DEBUG
Msg ("[%s]",_tmp[i]);
#endif // DEBUG
}
*/
#endif //#ifndef DEDICATED_SERVER
}
