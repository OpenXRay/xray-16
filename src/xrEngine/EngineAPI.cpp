// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

extern xr_vector<xr_token> VidQualityToken;

constexpr pcstr check_function = "CheckRendererSupport";
constexpr pcstr setup_function = "SetupEnv";
constexpr pcstr mode_function  = "GetModeName";

constexpr pcstr r1_library     = "xrRender_R1";
constexpr pcstr r2_library     = "xrRender_R2";
constexpr pcstr r3_library     = "xrRender_R3";
constexpr pcstr r4_library     = "xrRender_R4";
constexpr pcstr gl_library     = "xrRender_GL";

constexpr pcstr renderer_r1    = "renderer_r1";
constexpr pcstr renderer_r2a   = "renderer_r2a";
constexpr pcstr renderer_r2    = "renderer_r2";
constexpr pcstr renderer_r2_5  = "renderer_r2.5";
constexpr pcstr renderer_r3    = "renderer_r3";
constexpr pcstr renderer_r4    = "renderer_r4";
constexpr pcstr renderer_gl    = "renderer_gl";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void) {}

CEngineAPI::CEngineAPI()
{
    hGame = nullptr;
    hTuner = nullptr;
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
    return true; // In Linux allocated memory limited only by pointer size
#endif
}

void CEngineAPI::SelectRenderer()
{
    GEnv.CurrentRenderer = -1;

    const auto select = [&](pcstr library, u32 selected, int index, u32 fallback = 0)
    {
        if (psDeviceFlags.test(selected))
        {
            if (renderers[library]->IsLoaded())
            {
                GEnv.CurrentRenderer = index;
                setupSelectedRenderer = (SetupEnv)renderers[library]->GetProcAddress(setup_function);
            }
            else // Selected is unavailable
            {
                psDeviceFlags.set(selected, false);
                if (fallback > 0) // try to use another
                    psDeviceFlags.set(fallback, true);
            }
        }
    };

    select(gl_library, rsRGL, 5, rsR4);

#if defined(WINDOWS)
    select(r4_library, rsR4, 4, rsR3);
    select(r3_library, rsR3, 3, rsR2);
    select(r2_library, rsR2, 2, rsR1);
#endif

    select(r1_library, rsR1, 1);
}

void CEngineAPI::InitializeRenderers()
{
    SelectRenderer();

    if (setupSelectedRenderer == nullptr
        && VidQualityToken[0].id != -1)
    {
        // if engine failed to load renderer
        // but there is at least one available
        // then try again
        string64 buf;
        xr_sprintf(buf, "renderer %s", VidQualityToken[0].name);
        Console->Execute(buf);

        // Second attempt
        SelectRenderer();
    }

    // Ask current renderer to setup GEnv
    R_ASSERT2(setupSelectedRenderer, "Can't setup renderer");
    setupSelectedRenderer();

    // Now unload unused renderers
    // XXX: Unloading disabled due to typeids invalidation
    /*if (GEnv.CurrentRenderer != 5)
        renderers[gl_library]->close();

    if (GEnv.CurrentRenderer != 4)
        renderers[r4_library]->close();

    if (GEnv.CurrentRenderer != 3)
        renderers[r3_library]->close();

    if (GEnv.CurrentRenderer != 2)
        renderers[r2_library]->close();

    if (GEnv.CurrentRenderer != 1)
        renderers[r1_library]->close();*/
}

void CEngineAPI::Initialize(void)
{
    InitializeRenderers();

    hGame = XRay::LoadModule("xrGame");
    R_ASSERT2(hGame->IsLoaded(), "Game DLL raised exception during loading or there is no game DLL at all");

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
    pCreate = nullptr;
    pDestroy = nullptr;
    renderers.clear();
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

void CEngineAPI::CreateRendererList()
{
    if (!VidQualityToken.empty())
        return;

    renderers[gl_library] = XRay::LoadModule(gl_library);
#if defined(WINDOWS)
    renderers[r1_library] = XRay::LoadModule(r1_library);
#endif

    if (GEnv.isDedicatedServer)
    {
#if defined(WINDOWS)
        R_ASSERT2(renderers[r1_library]->IsLoaded(), "Dedicated server needs xrRender_R1 to work");
        VidQualityToken.emplace_back(renderer_r1, 0);
#elif defined(LINUX)
        R_ASSERT2(renderers[gl_library]->IsLoaded(), "Dedicated server needs xrRender_GL to work");
        VidQualityToken.emplace_back(renderer_gl, 0);
#endif
        VidQualityToken.emplace_back(nullptr, -1);
        return;
    }

    auto& modes = VidQualityToken;

#if defined(WINDOWS)
    // Hide "d3d10.dll not found" message box for XP
    SetErrorMode(SEM_FAILCRITICALERRORS);

    renderers[r2_library] = XRay::LoadModule(r2_library);
    renderers[r3_library] = XRay::LoadModule(r3_library);
    renderers[r4_library] = XRay::LoadModule(r4_library);

    // Restore error handling
    SetErrorMode(0);
#endif

    const auto checkRenderer = [&](pcstr library, pcstr mode, int index)
    {
        if (renderers[library]->IsLoaded())
        {
            // Load SupportCheck, SetupEnv and GetModeName functions from DLL
            const auto checkSupport = (SupportCheck)renderers[library]->GetProcAddress(check_function);
            const auto getModeName  = (GetModeName)renderers[library]->GetProcAddress(mode_function);

            // Test availability
            if (checkSupport && checkSupport())
                modes.emplace_back(getModeName ? getModeName() : mode, index);
            else // Close the handle if test is failed
                renderers[library]->Close();
        }
    };

#if defined(WINDOWS)
    checkRenderer(r1_library, renderer_r1, 0);
    if (renderers[r2_library]->IsLoaded())
    {
        modes.emplace_back(renderer_r2a, 1);
        modes.emplace_back(renderer_r2,  2);
    }
    checkRenderer(r2_library, renderer_r2_5, 3);
    checkRenderer(r3_library, renderer_r3,   4);
    checkRenderer(r4_library, renderer_r4,   5);
#endif

    checkRenderer(gl_library, renderer_gl, 6);

    modes.emplace_back(nullptr, -1);

    Msg("Available render modes[%d]:", modes.size());
    for (const auto& mode : modes)
        if (mode.name)
            Log(mode.name);
}
