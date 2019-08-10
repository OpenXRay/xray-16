#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

extern xr_vector<xr_token> vid_quality_token;

constexpr pcstr CHECK_FUNCTION = "CheckRendererSupport";
constexpr pcstr SETUP_FUNCTION = "SetupEnv";

constexpr pcstr R1_LIBRARY = "xrRender_R1";
constexpr pcstr R2_LIBRARY = "xrRender_R2";
constexpr pcstr R3_LIBRARY = "xrRender_R3";
constexpr pcstr R4_LIBRARY = "xrRender_R4";

constexpr pcstr RENDERER_R1 = "renderer_r1";
constexpr pcstr RENDERER_R2A = "renderer_r2a";
constexpr pcstr RENDERER_R2 = "renderer_r2";
constexpr pcstr RENDERER_R2_5 = "renderer_r2.5";
constexpr pcstr RENDERER_R3 = "renderer_r3";
constexpr pcstr RENDERER_R4 = "renderer_r4";

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

CEngineAPI::~CEngineAPI() { vid_quality_token.clear(); }

bool is_enough_address_space_available()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

void CEngineAPI::SelectRenderer()
{
    GEnv.CurrentRenderer = -1;

    const auto select = [&](pcstr library, u32 selected, int index, u32 fallback = 0)
    {
        if (psDeviceFlags.test(selected))
        {
            if (m_renderers[library]->IsLoaded())
            {
                GEnv.CurrentRenderer = index;
                m_setupSelectedRenderer = (SetupEnv)m_renderers[library]->GetProcAddress(SETUP_FUNCTION);
            }
            else // Selected is unavailable
            {
                psDeviceFlags.set(selected, false);
                if (fallback > 0) // try to use another
                    psDeviceFlags.set(fallback, true);
            }
        }
    };

    select(R4_LIBRARY, rsR4, 4, rsR3);
    select(R3_LIBRARY, rsR3, 3, rsR2);
    select(R2_LIBRARY, rsR2, 2, rsR1);
    select(R1_LIBRARY, rsR1, 1);
}

void CEngineAPI::InitializeRenderers()
{
    SelectRenderer();

    if (m_setupSelectedRenderer == nullptr && vid_quality_token[0].id != -1)
    {
        // if engine failed to load renderer
        // but there is at least one available
        // then try again
        string32 buf;
        xr_sprintf(buf, "renderer %s", vid_quality_token[0].name);
        Console->Execute(buf);

        // Second attempt
        SelectRenderer();
    }

    // ask current renderer to setup GEnv
    R_ASSERT2(m_setupSelectedRenderer, "Can't setup renderer");
    m_setupSelectedRenderer();

    Log("Selected renderer:", Console->GetString("renderer"));
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

    // Close only AFTER other libraries are loaded!!
    CloseUnusedLibraries();
}

void CEngineAPI::Destroy(void)
{
    hGame = nullptr;
    hTuner = nullptr;
    m_renderers.clear();
    pCreate = nullptr;
    pDestroy = nullptr;
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

void CEngineAPI::CloseUnusedLibraries()
{
    // Now unload unused renderers
    if (GEnv.CurrentRenderer != 4)
        m_renderers[R4_LIBRARY]->Close();

    if (GEnv.CurrentRenderer != 3)
        m_renderers[R3_LIBRARY]->Close();

    if (GEnv.CurrentRenderer != 2)
        m_renderers[R2_LIBRARY]->Close();

    if (GEnv.CurrentRenderer != 1)
        m_renderers[R1_LIBRARY]->Close();
}

void CEngineAPI::CreateRendererList()
{
    if (!vid_quality_token.empty())
        return;

    m_renderers[R1_LIBRARY] = XRay::LoadModule(R1_LIBRARY);
    m_renderers[R2_LIBRARY] = XRay::LoadModule(R2_LIBRARY);
    m_renderers[R3_LIBRARY] = XRay::LoadModule(R3_LIBRARY);
    m_renderers[R4_LIBRARY] = XRay::LoadModule(R4_LIBRARY);

    auto& modes = vid_quality_token;

    const auto checkRenderer = [&](pcstr library, pcstr mode, int index)
    {
        if (m_renderers[library]->IsLoaded())
        {
            // Load SupportCheck, SetupEnv and GetModeName functions from DLL
            const auto checkSupport = (SupportCheck)m_renderers[library]->GetProcAddress(CHECK_FUNCTION);

            // Test availability
            if (checkSupport && checkSupport())
                modes.emplace_back(mode, index);
            else // Close the handle if test is failed
                m_renderers[library]->Close();
        }
    };

    checkRenderer(R1_LIBRARY, RENDERER_R1, 0);
    if (m_renderers[R2_LIBRARY]->IsLoaded())
    {
        modes.emplace_back(RENDERER_R2A, 1);
        modes.emplace_back(RENDERER_R2, 2);
    }
    checkRenderer(R2_LIBRARY, RENDERER_R2_5, 3);
    checkRenderer(R3_LIBRARY, RENDERER_R3, 4);
    checkRenderer(R4_LIBRARY, RENDERER_R4, 5);

    modes.emplace_back(xr_token(nullptr, -1));

    Msg("Available render modes[%d]:", modes.size());
    for (const auto& mode : modes)
        if (mode.name)
            Log(mode.name);
}
