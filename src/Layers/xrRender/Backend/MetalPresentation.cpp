#include "stdafx.h"
#include "MetalPresentation.h"
#include "../FGRenderHost.h"
#include "../xrRender_console.h"
#include "xrEngine/IRenderBackend.h"
#include <SDL3/SDL.h>
#include <nvrhi/nvrhi.h>
#include <cstdio>

using namespace xray::render::fg;

namespace
{
struct PresentationRuntime
{
    SDL_Window* window = nullptr;
    IRenderBackend* backend = nullptr;
    bool coreInitialized = false;
    bool sdlInitialized = false;
    bool debugInitialized = false;

    ~PresentationRuntime()
    {
        if (backend)
        {
            backend->WaitForIdle();
            backend->Shutdown();
            GEnv.Backend = nullptr;
            xr_delete(backend);
        }
        if (window)
            SDL_DestroyWindow(window);
        if (coreInitialized)
            Core._destroy();
        if (sdlInitialized)
            SDL_Quit();
        if (debugInitialized)
            xrDebug::Finalize();
    }
};
}

int RunMetalPresentation(const char* commandLine)
{
    PresentationRuntime runtime;
    xrDebug::Initialize(commandLine);
    runtime.debugInitialized = true;
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::fprintf(stderr, "Metal presentation SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    runtime.sdlInitialized = true;
    Core.Initialize("OpenXRay", commandLine, false);
    runtime.coreInitialized = true;
    ps_fg_render_mode = FG_RENDER_METAL;
    runtime.window = SDL_CreateWindow("OpenXRay — Native Metal presentation | V: vsync | Esc: quit",
        960, 540, SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!runtime.window)
    {
        Msg("! [MetalPresentation] Window creation failed: %s", SDL_GetError());
        return 1;
    }
    u32 width = 960;
    u32 height = 540;
    runtime.backend = FGRenderHost::CreateBackend(runtime.window, width, height, strstr(commandLine, "-d3ddebug") != nullptr);
    if (!runtime.backend || runtime.backend->GetAPI() != IRenderBackend::API::Metal)
    {
        Msg("! [MetalPresentation] Native Metal backend creation failed");
        return 1;
    }
    SDL_ShowWindow(runtime.window);
    SDL_RaiseWindow(runtime.window);
    Msg("* [MetalPresentation] Presentation-only mode; game shaders and level rendering are not enabled");
    Msg("* [MetalPresentation] Native backend ready: %ux%u pixels. V toggles vsync; Escape closes the window", width, height);
    bool running = true;
    bool vsync = true;
    bool firstFrame = true;
    u64 frames = 0;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!event.key.repeat && event.key.key == SDLK_ESCAPE)
                    running = false;
                else if (!event.key.repeat && event.key.key == SDLK_V)
                {
                    vsync = !vsync;
                    Msg("* [MetalPresentation] Vsync %s", vsync ? "on" : "off");
                }
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_MINIMIZED:
                FGRenderHost::ResizeBackend(runtime.window, width, height);
                Msg("* [MetalPresentation] Window event %u: %ux%u pixels", event.type, width, height);
                break;
            default:
                break;
            }
        }
        if (!running)
            break;
        if (runtime.backend->GetDeviceState() == DeviceState::Lost)
        {
            Msg("! [MetalPresentation] Native device failed");
            return 1;
        }
        runtime.backend->BeginFrame();
        auto* backBuffer = runtime.backend->GetBackBuffer();
        if (!runtime.backend->IsInFrame() || !backBuffer)
        {
            SDL_Delay(16);
            continue;
        }
        auto* commands = runtime.backend->GetCommandList();
        commands->clearTextureFloat(backBuffer, nvrhi::AllSubresources, nvrhi::Color(0.035f, 0.22f, 0.32f, 1.f));
        runtime.backend->EndFrame();
        runtime.backend->Present(vsync);
        if (runtime.backend->GetDeviceState() == DeviceState::Lost)
        {
            Msg("! [MetalPresentation] Native frame submission failed");
            return 1;
        }
        ++frames;
        if (firstFrame)
        {
            Msg("* [MetalPresentation] First native frame queued for presentation");
            firstFrame = false;
        }
    }
    runtime.backend->WaitForIdle();
    if (runtime.backend->GetDeviceState() == DeviceState::Lost)
        return 1;
    Msg("* [MetalPresentation] Clean shutdown after %llu native frames", static_cast<unsigned long long>(frames));
    return 0;
}
