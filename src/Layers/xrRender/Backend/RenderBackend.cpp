#include "stdafx.h"
#include "RenderBackend.h"
#include "D3D11Backend.h"

#define ENABLE_D3D12_BACKEND
#ifdef ENABLE_D3D12_BACKEND
#include "D3D12Backend.h"
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#endif

namespace xray::render::ng {

xr_unique_ptr<IRenderBackend> CreateRenderBackend(GraphicsAPI api) {
    if (api == GraphicsAPI::Auto) {
        api = GetBestAvailableAPI();
        Msg("* [RenderBackend] Auto-selected: %s", GetGraphicsAPIName(api));
    }

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
            return true;

        case GraphicsAPI::D3D12: {
#ifdef ENABLE_D3D12_BACKEND
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
            return false;
#endif
        }

        case GraphicsAPI::Vulkan:
            return false;

        default:
            return false;
    }
}

GraphicsAPI GetBestAvailableAPI() {
#ifdef ENABLE_D3D12_BACKEND
    if (IsGraphicsAPIAvailable(GraphicsAPI::D3D12)) {
        return GraphicsAPI::D3D12;
    }
#endif

    return GraphicsAPI::D3D11;
}

}
