// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"

#include "xrCore/ModuleLookup.hpp"

extern xr_token* vid_quality_token;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void) {}

CEngineAPI::CEngineAPI()
{
    hGame = nullptr;
    hTuner = nullptr;
    hRenderR1 = nullptr;
    hRenderR2 = nullptr;
    hRenderR3 = nullptr;
    hRenderR4 = nullptr;
    pCreate = nullptr;
    pDestroy = nullptr;
    tune_enabled = false;
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

void CEngineAPI::InitializeNotDedicated()
{
    if (psDeviceFlags.test(rsR4))
    {
        
        if (hRenderR4->exist())
        {
            g_current_renderer = 4;
            GEnv.SetupCurrentRenderer = GEnv.SetupR4;
        }
        else
        {
            psDeviceFlags.set(rsR4, false);
            psDeviceFlags.set(rsR3, true);
        }
            
            
    }

    if (psDeviceFlags.test(rsR3))
    {
        if (hRenderR3->exist())
        {
            g_current_renderer = 3;
            GEnv.SetupCurrentRenderer = GEnv.SetupR3;
        }
        else
        {
            psDeviceFlags.set(rsR3, false);
            psDeviceFlags.set(rsR2, true);
        }
    }

    if (psDeviceFlags.test(rsR2))
    {
        
        if (hRenderR2->exist())
        {
            g_current_renderer = 2;
            GEnv.SetupCurrentRenderer = GEnv.SetupR2;
        }
        else
            psDeviceFlags.set(rsR2, false);
    }
}

void CEngineAPI::InitializeRenderers()
{    
    if (!GEnv.isDedicatedServer)
        InitializeNotDedicated();

    if (!psDeviceFlags.test(rsR4|rsR3|rsR2))
    {
        R_ASSERT(hRenderR1);
        renderer_value = 0; // con cmd
        g_current_renderer = 1;
        GEnv.SetupCurrentRenderer = GEnv.SetupR1;
    }

    // ask current renderer to setup GlobalEnv
    R_ASSERT(GEnv.SetupCurrentRenderer);
    GEnv.SetupCurrentRenderer();
}

void CEngineAPI::Initialize(void)
{
    hGame = std::make_unique<XRay::Module>("xrGame");

    if (!hGame->exist())
        R_CHK(GetLastError());
    R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

    pCreate = (Factory_Create*)hGame->getProcAddress("xrFactory_Create");
    R_ASSERT(pCreate);

    pDestroy = (Factory_Destroy*)hGame->getProcAddress("xrFactory_Destroy");
    R_ASSERT(pDestroy);

    //////////////////////////////////////////////////////////////////////////
    // vTune
    tune_enabled = false;
    if (strstr(Core.Params, "-tune"))
    {
        hTuner = std::make_unique<XRay::Module>("vTuneAPI");
        tune_pause = (VTPause*)hTuner->getProcAddress("VTPause");
        tune_resume = (VTResume*)hTuner->getProcAddress("VTResume");

        if (!tune_pause || !tune_pause)
        {
            Log("Can't initialize Intel vTune");
            tune_pause = dummy;
            tune_resume = dummy;
            return;
        }

        tune_enabled = true;
    }
}

void CEngineAPI::Destroy(void)
{
    if (hGame) hGame->close();
    if (hTuner) hTuner->close();
    if (hRenderR1) hRenderR1->close();
    if (hRenderR2) hRenderR2->close();
    if (hRenderR3) hRenderR3->close();
    if (hRenderR4) hRenderR4->close();
    pCreate = nullptr;
    pDestroy = nullptr;
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

void CEngineAPI::CreateRendererList()
{
    hRenderR1 = std::make_unique<XRay::Module>("xrRender_R1");

    if (GEnv.isDedicatedServer)
    {
        vid_quality_token = xr_alloc<xr_token>(2);

        vid_quality_token[0].id = 0;
        vid_quality_token[0].name = xr_strdup("renderer_r1");

        vid_quality_token[1].id = -1;
        vid_quality_token[1].name = nullptr;

        return;
    }

    if (vid_quality_token != nullptr)
        return;

    // Hide "d3d10.dll not found" message box for XP
    SetErrorMode(SEM_FAILCRITICALERRORS);

    hRenderR2 = std::make_unique<XRay::Module>("xrRender_R2");
    hRenderR3 = std::make_unique<XRay::Module>("xrRender_R3");
    hRenderR4 = std::make_unique<XRay::Module>("xrRender_R4");

    // Restore error handling
    SetErrorMode(0);

    bool bSupports_r2 = false;
    bool bSupports_r2_5 = false;
    bool bSupports_r3 = false;
    bool bSupports_r4 = false;

    if (strstr(Core.Params, "-perfhud_hack"))
    {
        bSupports_r2 = true;
        bSupports_r2_5 = true;
        bSupports_r3 = true;
        bSupports_r4 = true;
    }
    else
    {
        if (hRenderR2->exist())
        {
            bSupports_r2 = true;
            if (GEnv.CheckR2 && GEnv.CheckR2())
                bSupports_r2_5 = true;
        }
        if (hRenderR3->exist())
        {
            if (GEnv.CheckR3 && GEnv.CheckR3())
                bSupports_r3 = true;
            else
                hRenderR3->close();
        }
        if (hRenderR4->exist())
        {
            if (GEnv.CheckR4 && GEnv.CheckR4())
                bSupports_r4 = true;
            else
                hRenderR4->close();
        }
    }

    bool proceed = true;
    xr_vector<pcstr> tmp;
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

    Msg("Available render modes[%d]:", tmp.size());
    for (u32 i = 0; i < tmp.size(); ++i)
    {
        vid_quality_token[i].id = i;
        vid_quality_token[i].name = tmp[i];
        Msg("[%s]", tmp[i]);
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
}
