#include "stdafx.h"
#include "xr_3da/resource.h"
#include <SDL.h>

#include "Include/editor/ide.hpp"
#include "engine_impl.hpp"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/ModuleLookup.hpp"

void CRenderDevice::initialize_weather_editor()
{
    m_editor_module = XRay::LoadModule("xrWeatherEditor");
    if (!m_editor_module->IsLoaded())
        return;

    m_editor_initialize = (initialize_function_ptr)m_editor_module->GetProcAddress("initialize");
    VERIFY(m_editor_initialize);

    m_editor_finalize = (finalize_function_ptr)m_editor_module->GetProcAddress("finalize");
    VERIFY(m_editor_finalize);

    m_engine = new engine_impl();
    m_editor_initialize(m_editor, m_engine);
    VERIFY(m_editor);

    //m_hWnd = m_editor->view_handle();
    VERIFY(m_sdlWnd != INVALID_HANDLE_VALUE);

    GEnv.isEditor = true;
}

void CRenderDevice::Initialize()
{
    Log("Initializing Engine...");
    TimerGlobal.Start();
    TimerMM.Start();

    if (strstr(Core.Params, "-weather"))
        initialize_weather_editor();

    R_ASSERT3(SDL_Init(SDL_INIT_EVERYTHING) == 0, "Unable to initialize SDL", SDL_GetError());

    if (!m_sdlWnd)
    {
        Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
#if SDL_VERSION_ATLEAST(2, 0, 5)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#endif

        m_sdlWnd = SDL_CreateWindow("S.T.A.L.K.E.R.: Call of Pripyat", 0, 0, 0, 0, flags);
       
        R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());
    }
}

void CRenderDevice::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** ENGINE:   %2.2fms", stats.EngineTotal.result);
    font.OutNext("FPS/RFPS:     %3.1f/%3.1f", stats.fFPS, stats.fRFPS);
    font.OutNext("TPS:          %2.2f M", stats.fTPS);
    if (alert && stats.fFPS < 30)
        alert->Print(font, "FPS       < 30:   %3.1f", stats.fFPS);
}
