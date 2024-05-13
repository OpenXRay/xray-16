#include "stdafx.h"
#include "embedded_resources_management.h"

#include "xr_input.h"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/ModuleLookup.hpp"

#include <SDL3/SDL.h>
#ifdef IMGUI_ENABLE_VIEWPORTS
#   include <SDL3/SDL_syswm.h>
#endif

SDL_HitTestResult WindowHitTest(SDL_Window* win, const SDL_Point* area, void* data);

namespace
{
// This is put in a separate function due to bunch of defines.
// Keeping that in CRenderDevice::Initialize would harm the readability.
void SetSDLSettings(pcstr title)
{
#ifdef  SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif
#ifdef  SDL_HINT_AUDIO_DEVICE_APP_NAME
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, title);
#endif
#ifdef  SDL_HINT_APP_NAME
    SDL_SetHint(SDL_HINT_APP_NAME, title);
#endif
#ifdef  SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
#ifdef  SDL_HINT_MOUSE_AUTO_CAPTURE
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
#endif
}
} // namespace

void CRenderDevice::Initialize()
{
    ZoneScoped;
    Log("Initializing Engine...");
    TimerGlobal.Start();
    TimerMM.Start();

    {
        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN |
            SDL_WINDOW_RESIZABLE;

        GEnv.Render->ObtainRequiredWindowFlags(flags);

        int icon = IDI_ICON_COP;
        pcstr title = "S.T.A.L.K.E.R.: Call of Pripyat";

        if (ShadowOfChernobylMode)
        {
            icon = IDI_ICON_SOC;
            title = "S.T.A.L.K.E.R.: Shadow of Chernobyl";
        }
        else if (ClearSkyMode)
        {
            icon = IDI_ICON_CS;
            title = "S.T.A.L.K.E.R.: Clear Sky";
        }

        title = READ_IF_EXISTS(pSettingsOpenXRay, r_string_wb,
            "window", "title", title).c_str();

        xr_strcpy(Core.ApplicationTitle, title);
        SetSDLSettings(title);

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, 0);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, 0);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);
        SDL_SetNumberProperty(props, "flags", flags);
        m_sdlWnd = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);
        R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());

        SDL_SetWindowHitTest(m_sdlWnd, WindowHitTest, nullptr);
        SDL_SetWindowMinimumSize(m_sdlWnd, 256, 192);
        xrDebug::SetWindowHandler(this);
        ExtractAndSetWindowIcon(m_sdlWnd, icon);

        TracySetProgramName(title);
    }

#ifdef IMGUI_ENABLE_VIEWPORTS
    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for consistency, so that our code can use same logic for main and secondary viewports.
    {
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformUserData = IM_NEW(ImGuiViewportData){ m_sdlWnd };
        main_viewport->PlatformHandle = m_sdlWnd;
        main_viewport->PlatformHandleRaw = nullptr;
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(m_sdlWnd, &info))
        {
#if defined(SDL_VIDEO_DRIVER_WINDOWS /* SDL_VIDEO_DRIVER_WINDOWS has been removed in SDL3 */)
            main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
#elif defined(SDL_PLATFORM_APPLE) && defined(SDL_VIDEO_DRIVER_COCOA /* SDL_VIDEO_DRIVER_COCOA has been removed in SDL3 */)
            main_viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif
        }
    }
#endif

    if (!GEnv.isDedicatedServer)
    {
        seqAppStart.Add(&m_editor);
        seqAppEnd.Add(&m_editor);
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

void* CRenderDevice::GetApplicationWindowHandle() const
{
#if defined(XR_PLATFORM_WINDOWS)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(m_sdlWnd, &info))
        return info.info.win.window;
#endif
    return nullptr;
}
