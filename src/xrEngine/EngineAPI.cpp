// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

extern void FillMonitorsToken();
extern xr_vector<xr_token> VidQualityToken;

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
    hRenderRGL = nullptr;
    pCreate = nullptr;
    pDestroy = nullptr;
    tune_enabled = false;
    tune_pause = dummy;
    tune_resume = dummy;
}

CEngineAPI::~CEngineAPI()
{
    VidQualityToken.clear();
}

bool is_enough_address_space_available()
{
#if defined(WINDOWS)
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
#else
    return TRUE; // In linux allocated memory limited only by pointer size
#endif
}

void CEngineAPI::SetupCurrentRenderer()
{
    GEnv.CurrentRenderer = -1;

    if (psDeviceFlags.test(rsRGL))
    {
        if (hRenderRGL->IsLoaded())
        {
            GEnv.CurrentRenderer = 5;
            GEnv.SetupCurrentRenderer = GEnv.SetupRGL;
        }
        else
        {
            psDeviceFlags.set(rsRGL, false);
            psDeviceFlags.set(rsR4, true);
        }
    }

    if (psDeviceFlags.test(rsR4))
    {
        if (hRenderR4->IsLoaded())
        {
            GEnv.CurrentRenderer = 4;
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
        if (hRenderR3->IsLoaded())
        {
            GEnv.CurrentRenderer = 3;
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
        if (hRenderR2->IsLoaded())
        {
            GEnv.CurrentRenderer = 2;
            GEnv.SetupCurrentRenderer = GEnv.SetupR2;
        }
        else
        {
            psDeviceFlags.set(rsR2, false);
            psDeviceFlags.set(rsR1, true);
        }
    }

    if (psDeviceFlags.test(rsR1))
    {
        if (hRenderR1->IsLoaded())
        {
            GEnv.CurrentRenderer = 1;
            GEnv.SetupCurrentRenderer = GEnv.SetupR1;
        }
        else
            psDeviceFlags.set(rsR1, false);
    }
}

void CEngineAPI::InitializeRenderers()
{
    SetupCurrentRenderer();

    if (GEnv.SetupCurrentRenderer == nullptr
        && VidQualityToken[0].id != -1)
    {
        // if engine failed to load renderer
        // but there is at least one available
        // then try again
        string32 buf;
        xr_sprintf(buf, "renderer %s", VidQualityToken[0].name);
        Console->Execute(buf);

        // Second attempt
        SetupCurrentRenderer();
    }

    // ask current renderer to setup GlobalEnv
    R_ASSERT2(GEnv.SetupCurrentRenderer, "Can't setup renderer");
    GEnv.SetupCurrentRenderer();

    // Now unload unused renderers
    // XXX: Unloading disabled due to typeids invalidation
    /*if (GEnv.CurrentRenderer != 5)
        hRenderRGL->close();
    
    if (GEnv.CurrentRenderer != 4)
        hRenderR4->close();

    if (GEnv.CurrentRenderer != 3)
        hRenderR3->close();

    if (GEnv.CurrentRenderer != 2)
        hRenderR2->close();

    if (GEnv.CurrentRenderer != 1)
        hRenderR1->close();*/

    FillMonitorsToken();
}

void CEngineAPI::Initialize(void)
{
    InitializeRenderers();

    hGame = XRay::LoadModule("xrGame");
    R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

    pCreate = (Factory_Create*)hGame->GetProcAddress("xrFactory_Create");
    R_ASSERT(pCreate);

    pDestroy = (Factory_Destroy*)hGame->GetProcAddress("xrFactory_Destroy");
    R_ASSERT(pDestroy);

    //////////////////////////////////////////////////////////////////////////
    // vTune
    tune_enabled = false;
    if (strstr(Core.Params, "-tune"))
    {
        hTuner = XRay::LoadModule("vTuneAPI");
        tune_pause = (VTPause*)hTuner->GetProcAddress("VTPause");
        tune_resume = (VTResume*)hTuner->GetProcAddress("VTResume");

        if (!tune_pause || !tune_resume)
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
    hGame = nullptr;
    hTuner = nullptr;
    hRenderR1 = nullptr;
    hRenderR2 = nullptr;
    hRenderR3 = nullptr;
    hRenderR4 = nullptr;
    hRenderRGL = nullptr;
    pCreate = nullptr;
    pDestroy = nullptr;
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

void CEngineAPI::CreateRendererList()
{
    if (!VidQualityToken.empty())
        return;

    hRenderR1 = XRay::LoadModule("xrRender_R1");

    if (GEnv.isDedicatedServer)
    {
        R_ASSERT2(hRenderR1->IsLoaded(), "Dedicated server needs xrRender_R1 to work");
        VidQualityToken.emplace_back("renderer_r1", 0);
        VidQualityToken.emplace_back(nullptr, -1);
        return;
    }

    // Hide "d3d10.dll not found" message box for XP
    SetErrorMode(SEM_FAILCRITICALERRORS);

    hRenderR2 = XRay::LoadModule("xrRender_R2");
    hRenderR3 = XRay::LoadModule("xrRender_R3");
    hRenderR4 = XRay::LoadModule("xrRender_R4");
    hRenderRGL = XRay::LoadModule("xrRender_GL");

    // Restore error handling
    SetErrorMode(0);

    auto& modes = VidQualityToken;

    if (hRenderR1->IsLoaded())
    {
        modes.emplace_back("renderer_r1", 0);
    }

    if (hRenderR2->IsLoaded())
    {
        modes.emplace_back("renderer_r2a", 1);
        modes.emplace_back("renderer_r2", 2);
        if (GEnv.CheckR2 && GEnv.CheckR2())
            modes.emplace_back("renderer_r2.5", 3);
    }

    if (hRenderR3->IsLoaded())
    {
        if (GEnv.CheckR3 && GEnv.CheckR3())
            modes.emplace_back("renderer_r3", 4);
        else
            hRenderR3->Close();
    }

    if (hRenderR4->IsLoaded())
    {
        if (GEnv.CheckR4 && GEnv.CheckR4())
            modes.emplace_back("renderer_r4", 5);
        else
            hRenderR4->Close();
    }

    if (hRenderRGL->IsLoaded())
    {
        if (GEnv.CheckRGL && GEnv.CheckRGL())
            modes.emplace_back("renderer_gl", 6);
        else
            hRenderRGL->Close();
    }
    modes.emplace_back(nullptr, -1);

    Msg("Available render modes[%d]:", modes.size());
    for (const auto& mode : modes)
        if (mode.name)
            Log(mode.name);
}
