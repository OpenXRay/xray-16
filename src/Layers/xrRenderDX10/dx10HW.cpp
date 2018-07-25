#include "stdafx.h"

#include "Layers/xrRender/HW.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrCore/xr_token.h"

#include "StateManager/dx10SamplerStateCache.h"
#include "StateManager/dx10StateCache.h"

CHW HW;

CHW::CHW()
{
    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this);
}

CHW::~CHW()
{
    Device.seqAppActivate.Remove(this);
    Device.seqAppDeactivate.Remove(this);
}

void CHW::OnAppActivate()
{
    if (m_pSwapChain && !m_ChainDesc.Windowed)
    {
        ShowWindow(m_ChainDesc.OutputWindow, SW_RESTORE);
        m_pSwapChain->SetFullscreenState(TRUE, NULL);
    }
}

void CHW::OnAppDeactivate()
{
    if (m_pSwapChain && !m_ChainDesc.Windowed)
    {
        m_pSwapChain->SetFullscreenState(FALSE, NULL);
        ShowWindow(m_ChainDesc.OutputWindow, SW_MINIMIZE);
    }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
    // Минимально поддерживаемая версия Windows => Windows Vista SP2 или Windows 7.
    R_CHK(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&m_pFactory)));
    R_CHK(m_pFactory->EnumAdapters1(0, &m_pAdapter));
}

void CHW::DestroyD3D()
{
    _SHOW_REF("refCount:m_pAdapter", m_pAdapter);
    _RELEASE(m_pAdapter);

    _SHOW_REF("refCount:m_pFactory", m_pFactory);
    _RELEASE(m_pFactory);
}

void CHW::CreateDevice(SDL_Window* m_sdlWnd)
{
    CreateD3D();

    const bool bWindowed = !psDeviceFlags.is(rsFullscreen);

    m_DriverType = Caps.bForceGPU_REF ? D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

    // Display the name of video board
    DXGI_ADAPTER_DESC1 Desc;
    R_CHK(m_pAdapter->GetDesc1(&Desc));
    //  Warning: Desc.Description is wide string
    Msg("* GPU [vendor:%X]-[device:%X]: %S", Desc.VendorId, Desc.DeviceId, Desc.Description);

    Caps.id_vendor = Desc.VendorId;
    Caps.id_device = Desc.DeviceId;

    // Select back-buffer & depth-stencil format
    D3DFORMAT& fTarget = Caps.fTarget;
    D3DFORMAT& fDepth = Caps.fDepth;

    //  HACK: DX10: Embed hard target format.
    fTarget = D3DFMT_X8R8G8B8; //   No match in DX10. D3DFMT_A8B8G8R8->DXGI_FORMAT_R8G8B8A8_UNORM
    fDepth = selectDepthStencil(fTarget);

    // Set up the presentation parameters
    DXGI_SWAP_CHAIN_DESC& sd = m_ChainDesc;
    ZeroMemory(&sd, sizeof(sd));

    sd.BufferDesc.Width = Device.dwWidth;
    sd.BufferDesc.Height = Device.dwHeight;

    // Back buffer
    //. P.BackBufferWidth       = dwWidth;
    //. P.BackBufferHeight      = dwHeight;
    //  TODO: DX10: implement dynamic format selection
    // sd.BufferDesc.Format     = fTarget;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferCount = 1;

    // Multisample
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    // Windoze
    /* XXX:
       Probably the reason of weird tearing
       glitches reported by Shoker in windowed
       mode with VSync enabled.
       XXX: Fix this windoze stuff!!!
    */
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(m_sdlWnd, &info))
    {
        switch (info.subsystem)
        {
        case SDL_SYSWM_WINDOWS:
            sd.OutputWindow = info.info.win.window;
            break;
        default: break;
        }
    }
    else
        Log("Couldn't get window information: ", SDL_GetError());

    sd.Windowed = bWindowed;

    // Depth/stencil (DX10 don't need this?)
    //P.EnableAutoDepthStencil = TRUE;
    //P.AutoDepthStencilFormat = fDepth;
    //P.Flags = 0; //. D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

    //  Additional set up
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    UINT createDeviceFlags = 0;
#ifdef DEBUG
    // createDeviceFlags |= D3Dxx_CREATE_DEVICE_DEBUG;
#endif
    HRESULT R;
#ifdef USE_DX11
    D3D_FEATURE_LEVEL pFeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        // D3D_FEATURE_LEVEL_10_1,
        // D3D_FEATURE_LEVEL_10_0,
    };

    R = D3D11CreateDevice(m_pAdapter,
        D3D_DRIVER_TYPE_UNKNOWN, // Если мы выбираем конкретный адаптер, то мы обязаны использовать D3D_DRIVER_TYPE_UNKNOWN.
        NULL, createDeviceFlags, pFeatureLevels, sizeof(pFeatureLevels) / sizeof(pFeatureLevels[0]),
        D3D11_SDK_VERSION, &pDevice, &FeatureLevel, &pContext);
#else
    R = D3D10CreateDevice(m_pAdapter, m_DriverType, NULL, createDeviceFlags, D3D10_SDK_VERSION, &pDevice);

    pContext = pDevice;
    FeatureLevel = D3D_FEATURE_LEVEL_10_0;
    if (!FAILED(R))
    {
        D3DX10GetFeatureLevel1(pDevice, &pDevice1);
        FeatureLevel = D3D_FEATURE_LEVEL_10_1;
    }
    pContext1 = pDevice1;
#endif
    R_CHK(m_pFactory->CreateSwapChain(pDevice, &sd, &m_pSwapChain));

    if (FAILED(R))
    {
        // Fatal error! Cannot create rendering device AT STARTUP !!!
        Msg("Failed to initialize graphics hardware.\n"
            "Please try to restart the game.\n"
            "CreateDevice returned 0x%08x", R);
        FlushLog();
        MessageBox(nullptr, "Failed to initialize graphics hardware.\nPlease try to restart the game.", "Error!",
            MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
    };

    _SHOW_REF("* CREATE: DeviceREF:", HW.pDevice);

    //  Create render target and depth-stencil views here
    UpdateViews();

    const auto memory = Desc.DedicatedVideoMemory;
    Msg("*   Texture memory: %d M", memory / (1024 * 1024));
    //Msg("*        DDI-level: %2.1f", float(D3DXGetDriverLevel(pDevice)) / 100.f);
}

void CHW::DestroyDevice()
{
    //  Destroy state managers
    StateManager.Reset();
    RSManager.ClearStateArray();
    DSSManager.ClearStateArray();
    BSManager.ClearStateArray();
    SSManager.ClearStateArray();

    _SHOW_REF("refCount:pBaseZB", pBaseZB);
    _RELEASE(pBaseZB);

    _SHOW_REF("refCount:pBaseRT", pBaseRT);
    _RELEASE(pBaseRT);

    //  Must switch to windowed mode to release swap chain
    if (!m_ChainDesc.Windowed)
        m_pSwapChain->SetFullscreenState(FALSE, NULL);
    _SHOW_REF("refCount:m_pSwapChain", m_pSwapChain);
    _RELEASE(m_pSwapChain);

#ifdef USE_DX11
    _RELEASE(pContext);
#endif

#ifndef USE_DX11
    _RELEASE(HW.pDevice1);
#endif
    _SHOW_REF("refCount:HW.pDevice:", HW.pDevice);
    _RELEASE(HW.pDevice);

    DestroyD3D();
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset()
{
    DXGI_SWAP_CHAIN_DESC& cd = m_ChainDesc;
    const bool bWindowed = !psDeviceFlags.is(rsFullscreen);
    cd.Windowed = bWindowed;
    m_pSwapChain->SetFullscreenState(!bWindowed, NULL);
    DXGI_MODE_DESC& desc = m_ChainDesc.BufferDesc;
    desc.Width = Device.dwWidth;
    desc.Height = Device.dwHeight;

    CHK_DX(m_pSwapChain->ResizeTarget(&desc));

    _SHOW_REF("refCount:pBaseZB", pBaseZB);
    _SHOW_REF("refCount:pBaseRT", pBaseRT);
    _RELEASE(pBaseZB);
    _RELEASE(pBaseRT);
    CHK_DX(m_pSwapChain->ResizeBuffers(
        cd.BufferCount, desc.Width, desc.Height, desc.Format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    UpdateViews();
}

D3DFORMAT CHW::selectDepthStencil(D3DFORMAT /*fTarget*/)
{
// R3 hack
#pragma todo("R3 need to specify depth format")
    return D3DFMT_D24S8;
}

BOOL CHW::support(D3DFORMAT fmt, DWORD type, DWORD usage)
{
    // TODO: DX10: implement stub for this code.
    VERIFY(!"Implement CHW::support");
    return TRUE;
}

void CHW::UpdateViews()
{
    DXGI_SWAP_CHAIN_DESC& sd = m_ChainDesc;
    HRESULT R;

    // Create a render target view
    // R_CHK    (pDevice->GetRenderTarget           (0,&pBaseRT));
    ID3DTexture2D* pBuffer;
    R = m_pSwapChain->GetBuffer(0, __uuidof(ID3DTexture2D), (LPVOID*)&pBuffer);
    R_CHK(R);

    R = pDevice->CreateRenderTargetView(pBuffer, NULL, &pBaseRT);
    _RELEASE(pBuffer);
    R_CHK(R);

    //  Create Depth/stencil buffer
    //  HACK: DX10: hard depth buffer format
    // R_CHK    (pDevice->GetDepthStencilSurface    (&pBaseZB));
    ID3DTexture2D* pDepthStencil = NULL;
    D3D_TEXTURE2D_DESC descDepth;
    descDepth.Width = sd.BufferDesc.Width;
    descDepth.Height = sd.BufferDesc.Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D_USAGE_DEFAULT;
    descDepth.BindFlags = D3D_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    R = pDevice->CreateTexture2D(&descDepth, // Texture desc
        NULL, // Initial data
        &pDepthStencil); // [out] Texture
    R_CHK(R);

    //  Create Depth/stencil view
    R = pDevice->CreateDepthStencilView(pDepthStencil, NULL, &pBaseZB);
    R_CHK(R);

    _RELEASE(pDepthStencil);
}
