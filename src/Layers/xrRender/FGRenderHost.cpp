#include "stdafx.h"
#include "FGRenderHost.h"

#include "xrEngine/IRenderBackend.h"
#if defined(XR_PLATFORM_WINDOWS)
#include "Layers/xrRender/Backend/D3D12Backend.h"
#endif
#if defined(XRAY_USE_METAL)
#include "Layers/xrRender/Backend/MetalBackend.h"
#endif
#include "xrRender_console.h"

#include <SDL3/SDL.h>

using namespace xray::render::fg;

IRenderBackend* CreateVulkanBackend(SDL_Window* window, u32 width, u32 height, bool enableValidation);

IRenderBackend* FGRenderHost::CreateBackend(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, bool enableValidation)
{
    int w = 0, h = 0;
    if (ps_fg_render_mode == FG_RENDER_METAL)
    {
        if (!SDL_GetWindowSizeInPixels(hWnd, &w, &h))
        {
            FATAL("Cannot query Metal drawable pixel size");
            return nullptr;
        }
    }
    else
        SDL_GetWindowSize(hWnd, &w, &h);
    dwWidth = static_cast<u32>(w);
    dwHeight = static_cast<u32>(h);

    IRenderBackend* backend = nullptr;

    if (ps_fg_render_mode == FG_RENDER_VULKAN)
    {
        backend = CreateVulkanBackend(hWnd, dwWidth, dwHeight, enableValidation);
        if (!backend)
        {
            FATAL("Vulkan initialization failed");
            return nullptr;
        }
        Msg("* [FGRenderHost] Vulkan backend initialized successfully");
    }
    else if (ps_fg_render_mode == FG_RENDER_METAL)
    {
#if defined(XRAY_USE_METAL)
        backend = CreateMetalBackend(hWnd, dwWidth, dwHeight, enableValidation);
        if (!backend)
        {
            FATAL("Native Metal initialization failed - no fallback available");
            return nullptr;
        }
        Msg("* [FGRenderHost] Native Metal presentation backend initialized successfully");
#else
        FATAL("Native Metal backend is unavailable. Build on macOS with NVRHI_WITH_METAL3=ON.");
        return nullptr;
#endif
    }
    else if (ps_fg_render_mode == FG_RENDER_DX12)
    {
#if defined(XR_PLATFORM_WINDOWS)
        auto* dx12Backend = xr_new<D3D12Backend>();
        if (!dx12Backend->Initialize(hWnd, dwWidth, dwHeight, enableValidation))
        {
            Msg("! [FGRenderHost] D3D12 backend initialization failed");
            xr_delete(dx12Backend);
            FATAL("D3D12 initialization failed - no fallback available");
            return nullptr;
        }
        backend = dx12Backend;
        Msg("* [FGRenderHost] D3D12 backend initialized successfully");
#else
        FATAL("D3D12 backend not available on this platform");
        return nullptr;
#endif
    }
    else
    {
        FATAL("Unknown frame-graph backend selection");
        return nullptr;
    }

    Msg("*   Bindless textures: %s (max %u)",
        backend->GetCapabilities().bindlessTextures ? "Yes" : "No",
        backend->GetCapabilities().maxBindlessResources);

    GEnv.Backend = backend;
    std::tie(dwWidth, dwHeight) = backend->GetBackBufferSize();
    return backend;
}

void FGRenderHost::ResizeBackend(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight)
{
    if (!GEnv.Backend)
        return;

    int w = 0, h = 0;
    if (GEnv.Backend->GetAPI() == IRenderBackend::API::Metal)
    {
        if (!SDL_GetWindowSizeInPixels(hWnd, &w, &h))
        {
            Msg("! [FGRenderHost] Cannot query Metal drawable pixel size: %s", SDL_GetError());
            return;
        }
    }
    else
        SDL_GetWindowSize(hWnd, &w, &h);
    GEnv.Backend->ResizeSwapChain(static_cast<u32>(w), static_cast<u32>(h));
    std::tie(dwWidth, dwHeight) = GEnv.Backend->GetBackBufferSize();
}
