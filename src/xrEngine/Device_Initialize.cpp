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

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        Log("Unable to initialize SDL: %s", SDL_GetError());
    }

    if (!m_sdlWnd)
    {
        Uint32 flags = SDL_WINDOW_BORDERLESS;
#if SDL_VERSION_ATLEAST(2, 0, 5)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#endif

        if (strstr(Core.Params, "-gl"))
            flags |= SDL_WINDOW_OPENGL;

        m_sdlWnd = SDL_CreateWindow("S.T.A.L.K.E.R.: Call of Pripyat", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, flags);
       
        R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());
    }
    // Save window properties
    m_dwWindowStyle = SDL_GetWindowFlags(m_sdlWnd);

    SDL_GetWindowPosition(m_sdlWnd, &m_rcWindowClient.x, &m_rcWindowClient.y);
    int w = 0, h = 0;
    SDL_GetWindowSize(m_sdlWnd, &w, &h);
    m_rcWindowClient.w = m_rcWindowClient.x + w;
    m_rcWindowClient.h = m_rcWindowClient.y + h;
}

void CRenderDevice::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** ENGINE:   %2.2fms", stats.EngineTotal.result);
    font.OutNext("FPS/RFPS:     %3.1f/%3.1f", stats.fFPS, stats.fRFPS);
    font.OutNext("TPS:          %2.2f M", stats.fTPS);
    if (alert && stats.fFPS < 30)
        alert->Print(font, "FPS       < 30:   %3.1f", stats.fFPS);
}
