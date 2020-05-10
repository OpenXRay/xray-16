
// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

#include "xrScriptEngine/ScriptExporter.hpp"

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

static bool r2_available = false;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace
{
void __cdecl dummy(void) {}
IFactoryObject* __cdecl dummy(CLASS_ID) { return nullptr; }
template<typename T>
void __cdecl dummy(T* p) { R_ASSERT2(p == nullptr, "Attempting to release an object that shouldn't be allocated"); }
};

CEngineAPI::CEngineAPI()
{
    hGame = nullptr;
    hTuner = nullptr;
    pCreate = dummy;
    pDestroy = dummy;
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
#if defined(XR_PLATFORM_WINDOWS)
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

#if defined(XR_PLATFORM_WINDOWS)
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

    Log("Selected renderer:", Console->GetString("renderer"));
}

void CEngineAPI::Initialize(void)
{
    InitializeRenderers();

    hGame = XRay::LoadModule("xrGame");
    if (!CanSkipGameModuleLoading())
    {
        R_ASSERT2(hGame->IsLoaded(), "! Game DLL raised exception during loading or there is no game DLL at all");

        pCreate = reinterpret_cast<Factory_Create*>(hGame->GetProcAddress("xrFactory_Create"));
        R_ASSERT(pCreate);

        pDestroy = reinterpret_cast<Factory_Destroy*>(hGame->GetProcAddress("xrFactory_Destroy"));
        R_ASSERT(pDestroy);
    }

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

    CloseUnusedLibraries();
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

void CEngineAPI::CloseUnusedLibraries()
{
    // Only windows because on linux only one library is loaded - xrRender_GL
#ifdef XR_PLATFORM_WINDOWS
    if (GEnv.CurrentRenderer != 5)
        renderers[gl_library] = nullptr;

    if (GEnv.CurrentRenderer != 4)
        renderers[r4_library] = nullptr;

    if (GEnv.CurrentRenderer != 3)
        renderers[r3_library] = nullptr;

    if (GEnv.CurrentRenderer != 2)
        renderers[r2_library] = nullptr;

    if (GEnv.CurrentRenderer != 1)
        renderers[r1_library] = nullptr;
#endif
}

void CEngineAPI::CreateRendererList()
{
    if (!VidQualityToken.empty())
        return;

    renderers[gl_library] = XRay::LoadModule(gl_library);
#if defined(XR_PLATFORM_WINDOWS)
    renderers[r1_library] = XRay::LoadModule(r1_library);
#endif

    if (GEnv.isDedicatedServer)
    {
#if defined(XR_PLATFORM_WINDOWS)
        R_ASSERT2(renderers[r1_library]->IsLoaded(), "Dedicated server needs xrRender_R1 to work");
        VidQualityToken.emplace_back(renderer_r1, 0);
#elif defined(XR_PLATFORM_LINUX)
        R_ASSERT2(renderers[gl_library]->IsLoaded(), "Dedicated server needs xrRender_GL to work");
        VidQualityToken.emplace_back(renderer_gl, 0);
#endif
        VidQualityToken.emplace_back(nullptr, -1);
        return;
    }

    auto& modes = VidQualityToken;

#if defined(XR_PLATFORM_WINDOWS)
    renderers[r2_library] = XRay::LoadModule(r2_library);
    renderers[r3_library] = XRay::LoadModule(r3_library);
    renderers[r4_library] = XRay::LoadModule(r4_library);
#endif

    const auto checkRenderer = [&](pcstr library, pcstr mode, int index)
    {
        if (renderers[library]->IsLoaded())
        {
            // Load SupportCheck function from DLL
            const auto checkSupport = (SupportCheck)renderers[library]->GetProcAddress(check_function);

            // Test availability
            if (checkSupport && checkSupport())
                modes.emplace_back(mode, index);
        }
    };

#if defined(XR_PLATFORM_WINDOWS)
    checkRenderer(r1_library, renderer_r1, 0);
    if (renderers[r2_library]->IsLoaded())
    {
        r2_available = true;
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

bool is_r2_available()
{
    return r2_available;
}

SCRIPT_EXPORT(CheckRendererSupport, (),
{
    using namespace luabind;
    module(luaState)
    [
        def("xrRender_test_r2_hw", &is_r2_available)
    ];
});
