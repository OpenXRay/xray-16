#include "stdafx.h"

#ifdef IMGUI_ENABLE_VIEWPORTS
#   include <SDL3/SDL_syswm.h>
#endif

void CRenderDevice::InitializeImGui()
{
    if (m_imgui_context)
        return;

    ZoneScoped;

    ImGui::SetAllocatorFunctions(
        [](size_t size, void* /*user_data*/)
        {
            return xr_malloc(size);
        },
        [](void* ptr, void* /*user_data*/)
        {
            xr_free(ptr);
        }
    );
    m_imgui_context = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                      ImGuiConfigFlags_NavEnableGamepad |
                      ImGuiConfigFlags_NavEnableSetMousePos |
                      ImGuiConfigFlags_DockingEnable;

    string_path fName;
    FS.update_path(fName, "$app_data_root$", io.IniFilename);
    convert_path_separators(fName);
    io.IniFilename = xr_strdup(fName);

    FS.update_path(fName, "$logs$", io.LogFilename);
    io.LogFilename = xr_strdup(fName);

    io.BackendPlatformName = "OpenXRay";

    io.SetPlatformImeDataFn = [](ImGuiViewport* viewport, ImGuiPlatformImeData* data)
    {
        if (data->WantVisible)
        {
            /*const*/ SDL_Rect r // this is not const because older versions of SDL accept non-const rect
            {
                /*.x =*/ (int)(data->InputPos.x - viewport->Pos.x),
                /*.y =*/ (int)(data->InputPos.y - viewport->Pos.y + data->InputLineHeight),
                /*.w =*/ 1,
                /*.h =*/ (int)data->InputLineHeight,
            };
            SDL_SetTextInputRect(&r);
        }
    };

#ifdef IMGUI_ENABLE_VIEWPORTS
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    platform_io.Platform_CreateWindow = [](ImGuiViewport* viewport)
    {
        Uint32 sdl_flags{};
        GEnv.Render->ObtainRequiredWindowFlags(sdl_flags);

        //sdl_flags |= SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_HIGH_PIXEL_DENSITY; // XXX: high DPI
        sdl_flags |= SDL_WINDOW_HIDDEN;
        sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? SDL_WINDOW_BORDERLESS : 0;
        sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : SDL_WINDOW_RESIZABLE;
#if !defined(XR_PLATFORM_WINDOWS)
        // See SDL hack in ImGui_ImplSDL2_ShowWindow().
        sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) ? SDL_WINDOW_UTILITY : 0;
#endif
        sdl_flags |= (viewport->Flags & ImGuiViewportFlags_TopMost) ? SDL_WINDOW_ALWAYS_ON_TOP : 0;

        const auto vd = IM_NEW(ImGuiViewportData)
        {
            viewport->Pos, viewport->Size, sdl_flags
        };
        viewport->PlatformUserData = vd;

        viewport->PlatformHandle = vd->Window;
        viewport->PlatformHandleRaw = nullptr;

        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(vd->Window, &info))
        {
#if defined(XR_PLATFORM_WINDOWS) && defined(SDL_VIDEO_DRIVER_WINDOWS /* SDL_VIDEO_DRIVER_WINDOWS has been removed in SDL3 */)
            viewport->PlatformHandleRaw = info.info.win.window;
#elif defined(SDL_PLATFORM_APPLE) && defined(SDL_VIDEO_DRIVER_COCOA /* SDL_VIDEO_DRIVER_COCOA has been removed in SDL3 */)
            viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif
        }

        if (viewport->ParentViewportId)
        {
            const auto parentViewport = ImGui::FindViewportByID(viewport->ParentViewportId);
            SDL_SetWindowModalFor(vd->Window, (SDL_Window*)parentViewport->PlatformHandle);
        }
    };

    platform_io.Platform_DestroyWindow = [](ImGuiViewport* viewport)
    {
        if (const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData))
        {
            IM_DELETE(vd);
        }
        viewport->PlatformUserData = nullptr;
        viewport->PlatformHandle = nullptr;
        viewport->PlatformHandleRaw = nullptr;
    };

    platform_io.Platform_ShowWindow = [](ImGuiViewport* viewport)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
#if defined(XR_PLATFORM_WINDOWS)
        const HWND hwnd = static_cast<HWND>(viewport->PlatformHandleRaw);

        // SDL hack: Hide icon from task bar
        // Note: SDL 2.0.6+ has a SDL_WINDOW_UTILITY flag which is supported under Windows but the way it create the window breaks our seamless transition.
        if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
        {
            LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
            ex_style &= ~WS_EX_APPWINDOW;
            ex_style |= WS_EX_TOOLWINDOW;
            ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
        }

        // SDL hack: SDL always activate/focus windows :/
        if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
        {
            ::ShowWindow(hwnd, SW_SHOWNA);
            return;
        }
#endif
        SDL_ShowWindow(vd->Window);
    };

    platform_io.Platform_SetWindowPos = [](ImGuiViewport* viewport, ImVec2 pos)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        SDL_SetWindowPosition(vd->Window, (int)pos.x, (int)pos.y);
    };

    platform_io.Platform_GetWindowPos = [](ImGuiViewport* viewport)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        int x = 0, y = 0;
        SDL_GetWindowPosition(vd->Window, &x, &y);
        return ImVec2{ (float)x, (float)y };
    };

    platform_io.Platform_SetWindowSize = [](ImGuiViewport* viewport, ImVec2 size)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        SDL_SetWindowSize(vd->Window, (int)size.x, (int)size.y);
    };

    platform_io.Platform_GetWindowSize = [](ImGuiViewport* viewport)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        int w = 0, h = 0;
        SDL_GetWindowSize(vd->Window, &w, &h);
        return ImVec2{ (float)w, (float)h };
    };

    platform_io.Platform_SetWindowFocus = [](ImGuiViewport* viewport)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        SDL_RaiseWindow(vd->Window);
    };

    platform_io.Platform_GetWindowFocus = [](ImGuiViewport* viewport)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        return (SDL_GetWindowFlags(vd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    };

    platform_io.Platform_GetWindowMinimized = [](ImGuiViewport* viewport)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        return (SDL_GetWindowFlags(vd->Window) & SDL_WINDOW_MINIMIZED) != 0;
    };

    platform_io.Platform_SetWindowTitle = [](ImGuiViewport* viewport, const char* title)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        SDL_SetWindowTitle(vd->Window, title);
    };

    platform_io.Platform_SetWindowAlpha = [](ImGuiViewport* viewport, float alpha)
    {
        const auto vd = static_cast<ImGuiViewportData*>(viewport->PlatformUserData);
        SDL_SetWindowOpacity(vd->Window, alpha);
    };
#endif // IMGUI_ENABLE_VIEWPORTS

    editor().InitBackend();

    if (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports)
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
}

void CRenderDevice::DestroyImGui()
{
    if (!m_imgui_context)
        return;

    ZoneScoped;

    m_imgui_render->OnDeviceDestroy();
    GEnv.RenderFactory->DestroyImGuiRender(m_imgui_render);
    m_imgui_render = nullptr;

#ifdef IMGUI_ENABLE_VIEWPORTS
    ImGui::DestroyPlatformWindows();
#endif
    editor().ShutdownBackend();

    ImGuiIO& io = ImGui::GetIO();
    xr_free(io.IniFilename);
    xr_free(io.LogFilename);

    ImGui::DestroyContext(m_imgui_context);
    m_imgui_context = nullptr;
}
