#include "stdafx.h"
#include "embedded_resources_management.h"
#include "SDL.h"

#include "Include/editor/ide.hpp"
#include "xr_input.h"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/ModuleLookup.hpp"

SDL_HitTestResult WindowHitTest(SDL_Window* win, const SDL_Point* area, void* data);

void CRenderDevice::initialize_weather_editor()
{
    m_editor_module = XRay::LoadModule("xrWeatherEditor");
    if (!m_editor_module->IsLoaded())
        return;

    m_editor_initialize = (initialize_function_ptr)m_editor_module->GetProcAddress("initialize");
    VERIFY(m_editor_initialize);

    m_editor_finalize = (finalize_function_ptr)m_editor_module->GetProcAddress("finalize");
    VERIFY(m_editor_finalize);
#if defined(XR_PLATFORM_WINDOWS)
    m_editor_initialize(m_editor);
#endif
    VERIFY(m_editor);

    m_sdlWnd = SDL_CreateWindowFrom(m_editor->view_handle());
    R_ASSERT3(m_sdlWnd, "Unable to create SDL window from editor", SDL_GetError());

    GEnv.isEditor = true;
}

void CRenderDevice::Initialize()
{
    Log("Initializing Engine...");
    TimerGlobal.Start();
    TimerMM.Start();

    if (strstr(Core.Params, "-weather"))
        initialize_weather_editor();

    if (!m_sdlWnd)
    {
        Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN |
            SDL_WINDOW_RESIZABLE;

        GEnv.Render->ObtainRequiredWindowFlags(flags);

        int icon = IDI_COP;
        pcstr title = "S.T.A.L.K.E.R.: Call of Pripyat";

        if (ShadowOfChernobylMode)
        {
            icon = IDI_SOC;
            title = "S.T.A.L.K.E.R.: Shadow of Chernobyl";
        }
        else if (ClearSkyMode)
        {
            icon = IDI_CS;
            title = "S.T.A.L.K.E.R.: Clear Sky";
        }

        title = READ_IF_EXISTS(pSettingsOpenXRay, r_string,
            "window", "title", title);

        xr_strcpy(Core.ApplicationTitle, title);
        m_sdlWnd = SDL_CreateWindow(title, 0, 0, 640, 480, flags);
        R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());
        SDL_SetWindowHitTest(m_sdlWnd, WindowHitTest, nullptr);
        SDL_SetWindowMinimumSize(m_sdlWnd, 256, 192);
        xrDebug::SetWindowHandler(this);
        ExtractAndSetWindowIcon(m_sdlWnd, icon);
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

SDL_HitTestResult WindowHitTest(SDL_Window* /*window*/, const SDL_Point* pArea, void* /*data*/)
{
    if (!Device.IsWindowDraggable())
        return SDL_HITTEST_NORMAL;

    SDL_Point area = *pArea; // copy
    const auto& rect = Device.m_rcWindowClient;

    // size of additional interactive area (in pixels)
    constexpr int hit = 15;
    constexpr int fix = 65535; // u32(-1)

    // Workaround for SDL bug 
    if (area.x + hit >= fix && rect.w <= fix - hit)
        area.x -= fix;

    const bool leftSide = area.x <= rect.x + hit;
    const bool topSide = area.y <= rect.y + hit;
    const bool bottomSide = area.y >= rect.h - hit;
    const bool rightSide = area.x >= rect.w - hit;

    if (leftSide && topSide)
        return SDL_HITTEST_RESIZE_TOPLEFT;

    if (rightSide && topSide)
        return SDL_HITTEST_RESIZE_TOPRIGHT;

    if (rightSide && bottomSide)
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

    if (leftSide && bottomSide)
        return SDL_HITTEST_RESIZE_BOTTOMLEFT;

    if (topSide)
        return SDL_HITTEST_RESIZE_TOP;

    if (rightSide)
        return SDL_HITTEST_RESIZE_RIGHT;

    if (bottomSide)
        return SDL_HITTEST_RESIZE_BOTTOM;

    if (leftSide)
        return SDL_HITTEST_RESIZE_LEFT;

    return SDL_HITTEST_DRAGGABLE;
}
