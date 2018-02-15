#include "stdafx.h"
#include "xrCore/ModuleLookup.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool TestDX11Present()
{
    // Register class
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.lpszClassName = "TestDX11WindowClass";
    if (!RegisterClassEx(&wcex))
    {
        Log("* DX11: failed to register window class");
        return false;
    }

    // Create window
    HWND hWnd = CreateWindow("TestDX11WindowClass", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

    if (!hWnd)
    {
        Msg("* DX11: failed to create window");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL pFeatureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL FeatureLevel;

    ID3D11Device* pd3dDevice = NULL;
    ID3D11DeviceContext* pContext = NULL;
    IDXGISwapChain* pSwapChain = NULL;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, pFeatureLevels, 1, D3D11_SDK_VERSION,
        &sd, &pSwapChain, &pd3dDevice, &FeatureLevel, &pContext);

    if (FAILED(hr))
        Msg("* D3D11: device creation failed with hr=0x%08x", hr);

    _RELEASE(pSwapChain);
    _RELEASE(pd3dDevice);
    _RELEASE(pContext);

    DestroyWindow(hWnd);
    UnregisterClass("TestDX11WindowClass", GetModuleHandle(NULL));

    return SUCCEEDED(hr);
}

BOOL xrRender_test_hw()
{
    // CHW							_HW;
    // HRESULT						hr;
    //_HW.CreateD3D				()		;
    // hr = _HW.m_pAdapter->CheckInterfaceSupport(__uuidof(ID3DDevice), 0);
    //_HW.DestroyD3D				()		;

    return TestDX11Present(); // SUCCEEDED(hr);
}
