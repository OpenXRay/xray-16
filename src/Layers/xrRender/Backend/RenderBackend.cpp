// RenderBackend.cpp
// Backend factory and global accessor implementation
// This replaces the legacy HW global from CHW
#include "stdafx.h"
#include "RenderBackend.h"
#include "D3D11Backend.h"

// D3D12 backend - requires DirectX-Headers package (added as submodule)
#define ENABLE_D3D12_BACKEND
#ifdef ENABLE_D3D12_BACKEND
#include "D3D12Backend.h"
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#endif

namespace xray::render::ng {

// ═══════════════════════════════════════════════════════
//  BACKEND FACTORY
// ═══════════════════════════════════════════════════════

xr_unique_ptr<IRenderBackend> CreateRenderBackend(GraphicsAPI api) {
    // Handle auto-selection
    if (api == GraphicsAPI::Auto) {
        api = GetBestAvailableAPI();
        Msg("* [RenderBackend] Auto-selected: %s", GetGraphicsAPIName(api));
    }

    // Note: Using xr_unique_ptr with reset() because the custom deleter
    // doesn't support implicit derived-to-base conversion with xr_make_unique
    xr_unique_ptr<IRenderBackend> result;

    switch (api) {
        case GraphicsAPI::D3D11:
            Msg("* [RenderBackend] Creating D3D11 backend (legacy, no bindless)");
            result.reset(xr_new<D3D11Backend>());
            break;

        case GraphicsAPI::D3D12:
#ifdef ENABLE_D3D12_BACKEND
            if (IsGraphicsAPIAvailable(GraphicsAPI::D3D12)) {
                Msg("* [RenderBackend] Creating D3D12 backend (bindless enabled)");
                result.reset(xr_new<D3D12Backend>());
                break;
            }
#endif
            Msg("! [RenderBackend] D3D12 not available, falling back to D3D11");
            result.reset(xr_new<D3D11Backend>());
            break;

        case GraphicsAPI::Vulkan:
            Msg("! [RenderBackend] Vulkan backend not yet implemented");
            break;

        default:
            Msg("! [RenderBackend] Unknown graphics API");
            break;
    }

    return result;
}

bool IsGraphicsAPIAvailable(GraphicsAPI api) {
    switch (api) {
        case GraphicsAPI::D3D11:
            // D3D11 is always available on Windows 7+
            return true;

        case GraphicsAPI::D3D12: {
#ifdef ENABLE_D3D12_BACKEND
            // Check if D3D12 is available by trying to create a device
            static int s_d3d12Available = -1;
            if (s_d3d12Available == -1) {
                ID3D12Device* testDevice = nullptr;
                HRESULT hr = D3D12CreateDevice(
                    nullptr,
                    D3D_FEATURE_LEVEL_12_0,
                    IID_PPV_ARGS(&testDevice)
                );
                s_d3d12Available = SUCCEEDED(hr) ? 1 : 0;
                if (testDevice) {
                    testDevice->Release();
                }
                Msg("* [RenderBackend] D3D12 availability check: %s",
                    s_d3d12Available ? "Available" : "Not available");
            }
            return s_d3d12Available == 1;
#else
            // D3D12 backend not compiled in
            return false;
#endif
        }

        case GraphicsAPI::Vulkan:
            // TODO: Check Vulkan availability via vkEnumerateInstanceVersion
            return false;

        default:
            return false;
    }
}

GraphicsAPI GetBestAvailableAPI() {
#ifdef ENABLE_D3D12_BACKEND
    // Prefer D3D12 for true bindless texture support
    if (IsGraphicsAPIAvailable(GraphicsAPI::D3D12)) {
        return GraphicsAPI::D3D12;
    }
#endif

    // TODO: Check Vulkan when implemented
    // if (IsGraphicsAPIAvailable(GraphicsAPI::Vulkan)) {
    //     return GraphicsAPI::Vulkan;
    // }

    // Fall back to D3D11 (no true bindless, uses atlas workaround)
    return GraphicsAPI::D3D11;
}

} // namespace xray::render::ng
