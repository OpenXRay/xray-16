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

void __cdecl dummy(void)
{
};
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

extern u32 renderer_value; //con cmd
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
    LPCSTR gl_name = "xrRender_GL";
    LPCSTR r2_name = "xrRender_R2";
    LPCSTR r3_name = "xrRender_R3";
    LPCSTR r4_name = "xrRender_R4";

    if (psDeviceFlags.test(rsGL))
    {
        // try to initialize GL
        Log("Loading DLL:", gl_name);
        hRender = LoadLibrary(gl_name);
        if (0 == hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware.");
            psDeviceFlags.set(rsR2, TRUE);
        }
    }

    if (psDeviceFlags.test(rsR4))
    {
        // try to initialize R4
        hRender = XRay::LoadLibrary(r4_name);
        if (0 == hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware/pre-Vista OS.");
            psDeviceFlags.set(rsR2, TRUE);
        }
    }

    if (psDeviceFlags.test(rsR3))
    {
        // try to initialize R3
        hRender = XRay::LoadLibrary(r3_name);
        if (0 == hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware/pre-Vista OS.");
            psDeviceFlags.set(rsR2, TRUE);
        }
        else
            g_current_renderer = 3;
    }

    if (psDeviceFlags.test(rsR2))
    {
        // try to initialize R2
        psDeviceFlags.set(rsR4, FALSE);
        psDeviceFlags.set(rsR3, FALSE);
        hRender = XRay::LoadLibrary(r2_name);
        if (0 == hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware.");
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
    LPCSTR r1_name = "xrRender_R1";

#ifndef DEDICATED_SERVER
    InitializeNotDedicated();
#endif // DEDICATED_SERVER

    if (0 == hRender)
    {
        // try to load R1
        psDeviceFlags.set(rsR4, FALSE);
        psDeviceFlags.set(rsR3, FALSE);
        psDeviceFlags.set(rsR2, FALSE);
        renderer_value = 0; //con cmd

        hRender = XRay::LoadLibrary(r1_name);
        if (0 == hRender) R_CHK(GetLastError());
        R_ASSERT(hRender);
        g_current_renderer = 1;
    }
    // ask current renderer to setup GlobalEnv
    using SetupEnvFunc = void(*)();
    auto setupEnv = (SetupEnvFunc)XRay::GetProcAddress(hRender, "SetupEnv");
    R_ASSERT(setupEnv);
    setupEnv();
    // game
    {
        LPCSTR g_name = "xrGame";
        hGame = XRay::LoadLibrary(g_name);
        if (0 == hGame) R_CHK(GetLastError());
        R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");
        pCreate = (Factory_Create*)XRay::GetProcAddress(hGame, "xrFactory_Create");
        R_ASSERT(pCreate);
        pDestroy = (Factory_Destroy*)XRay::GetProcAddress(hGame, "xrFactory_Destroy");
        R_ASSERT(pDestroy);
    }

    //////////////////////////////////////////////////////////////////////////
    // vTune
    tune_enabled = FALSE;
    if (strstr(Core.Params, "-tune"))
    {
        LPCSTR g_name = "vTuneAPI";
        hTuner = XRay::LoadLibrary(g_name);
        if (0 == hTuner) R_CHK(GetLastError());
        R_ASSERT2(hTuner, "Intel vTune is not installed");
        tune_enabled = TRUE;
        tune_pause = (VTPause*)XRay::GetProcAddress(hTuner, "VTPause");
        R_ASSERT(tune_pause);
        tune_resume = (VTResume*)XRay::GetProcAddress(hTuner, "VTResume");
        R_ASSERT(tune_resume);
    }
}

void CEngineAPI::Destroy(void)
{
    if (hGame) { XRay::UnloadLibrary(hGame); hGame = 0; }
    if (hRender) { XRay::UnloadLibrary(hRender); hRender = 0; }
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
    if (vid_quality_token != NULL) return;
    bool bSupports_gl = false;
    bool bSupports_r2 = false;
    bool bSupports_r2_5 = false;
    bool bSupports_r3 = false;
    bool bSupports_r4 = false;

    LPCSTR gl_name = "xrRender_GL";
    LPCSTR r2_name = "xrRender_R2";
    LPCSTR r3_name = "xrRender_R3";
    LPCSTR r4_name = "xrRender_R4";

    if (strstr(Core.Params, "-perfhud_hack"))
    {
        bSupports_gl = true;
        bSupports_r2 = true;
        bSupports_r2_5 = true;
        bSupports_r3 = true;
        bSupports_r4 = true;
    }
    else
    {
        // XXX: since we are going to support OpenGL render with its own feature levels,
        // the reference render availability checking trick doesn't quite work: it's based
        // on assumption that first unsupported render quality level means all the rest
        // (greater) levels are not supported too, which is incorrect in case of Linux,
        // where we have OpenGL only (so the engine would crash on R_ASSERT below).
        // ...
        // try to initialize R2
        hRender = XRay::LoadLibrary(r2_name);
        if (hRender)
        {
            bSupports_r2 = true;
            SupportsAdvancedRendering* test_rendering = (SupportsAdvancedRendering*)XRay::GetProcAddress(hRender, "SupportsAdvancedRendering");
            R_ASSERT(test_rendering);
            bSupports_r2_5 = test_rendering();
        }

        // try to initialize R3
        Log("Loading DLL:", r3_name);
        // Hide "d3d10.dll not found" message box for XP
        SetErrorMode(SEM_FAILCRITICALERRORS);
        hRender = XRay::LoadLibrary(r3_name);
        // Restore error handling
        SetErrorMode(0);
        if (hRender)
        {
            SupportsDX10Rendering* test_dx10_rendering = (SupportsDX10Rendering*)XRay::GetProcAddress(hRender, "SupportsDX10Rendering");
            R_ASSERT(test_dx10_rendering);
            bSupports_r3 = test_dx10_rendering();
        }

        // try to initialize R4
        Log("Loading DLL:", r4_name);
        // Hide "d3d10.dll not found" message box for XP
        SetErrorMode(SEM_FAILCRITICALERRORS);
        hRender = XRay::LoadLibrary(r4_name);
        // Restore error handling
        SetErrorMode(0);
        if (hRender)
        {
            SupportsDX11Rendering* test_dx11_rendering = (SupportsDX11Rendering*)XRay::GetProcAddress(hRender, "SupportsDX11Rendering");
            R_ASSERT(test_dx11_rendering);
            bSupports_r4 = test_dx11_rendering();
        }

        // try to initialize GL
        Log("Loading DLL:", gl_name);
        hRender = LoadLibrary(gl_name);
        if (hRender)
            bSupports_gl = true;
    }

    hRender = 0;
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
    if (proceed &= bSupports_gl, proceed)
        tmp.push_back("renderer_gl");
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
