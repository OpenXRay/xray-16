#include "stdafx.h"
#include "embedded_resources_management.h"

#include "xr_input.h"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/ModuleLookup.hpp"

#include <SDL3/SDL.h>
#ifdef IMGUI_ENABLE_VIEWPORTS
#endif


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
        Uint32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;

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

        m_sdlWnd = SDL_CreateWindow(title, 640, 480, flags);
        R_ASSERT3(m_sdlWnd, "Unable to create SDL window", SDL_GetError());

        SDL_SetWindowMinimumSize(m_sdlWnd, 256, 192);
        xrDebug::SetWindowHandler(this);
        ExtractAndSetWindowIcon(m_sdlWnd, icon);
    }

#ifdef IMGUI_ENABLE_VIEWPORTS
    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for consistency, so that our code can use same logic for main and secondary viewports.
    {
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformUserData = IM_NEW(ImGuiViewportData){ m_sdlWnd };
        main_viewport->PlatformHandle = m_sdlWnd;
        main_viewport->PlatformHandleRaw = nullptr;
        SDL_PropertiesID props = SDL_GetWindowProperties(m_sdlWnd);
#if defined(XR_PLATFORM_WINDOWS)
        main_viewport->PlatformHandleRaw = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__APPLE__)
        main_viewport->PlatformHandleRaw = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#endif
    }
#endif

    if (!GEnv.isDedicatedServer)
    {
        seqFrame.Add(&m_editor, -5);
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

void* CRenderDevice::GetApplicationWindowHandle() const
{
#if defined(XR_PLATFORM_WINDOWS)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(m_sdlWnd), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
    return nullptr;
#endif
}
