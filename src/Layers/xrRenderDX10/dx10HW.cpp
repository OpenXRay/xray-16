#include "stdafx.h"

#include "Layers/xrRender/HW.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrCore/xr_token.h"

#include "StateManager/dx10SamplerStateCache.h"
#include "StateManager/dx10StateCache.h"

#ifndef _EDITOR
void fill_vid_mode_list(CHW* _hw);
void free_vid_mode_list();

void fill_render_mode_list();
void free_render_mode_list();
#else
void fill_vid_mode_list(CHW* _hw) {}
void free_vid_mode_list() {}
void fill_render_mode_list() {}
void free_render_mode_list() {}
#endif

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

void CHW::CreateDevice(HWND m_hWnd, bool move_window)
{
    m_move_window = move_window;
    CreateD3D();

    if (strstr(Core.Params, "-wnd_mode"))
        psDeviceFlags.set(rsFullscreen, false);

    bool bWindowed = !psDeviceFlags.is(rsFullscreen);

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

    selectResolution(sd.BufferDesc.Width, sd.BufferDesc.Height, bWindowed);

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
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.OutputWindow = m_hWnd;
    sd.Windowed = bWindowed;

    // Depth/stencil (DX10 don't need this?)
    //P.EnableAutoDepthStencil = TRUE;
    //P.AutoDepthStencilFormat = fDepth;
    //P.Flags = 0; //. D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

    // Refresh rate
    if (bWindowed)
    {
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
    }
    else
    {
        sd.BufferDesc.RefreshRate = selectRefresh(sd.BufferDesc.Width, sd.BufferDesc.Height, sd.BufferDesc.Format);
    }

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

    size_t memory = Desc.DedicatedVideoMemory;
    Msg("*   Texture memory: %d M", memory / (1024 * 1024));
    //Msg("*        DDI-level: %2.1f", float(D3DXGetDriverLevel(pDevice)) / 100.f);
#ifndef _EDITOR
    updateWindowProps(m_hWnd);
    fill_vid_mode_list(this);
#endif
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

#ifndef _EDITOR
    free_vid_mode_list();
#endif
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC& cd = m_ChainDesc;

    if (strstr(Core.Params, "-wnd_mode"))
        psDeviceFlags.set(rsFullscreen, false);

    BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);
    cd.Windowed = bWindowed;
    m_pSwapChain->SetFullscreenState(!bWindowed, NULL);
    DXGI_MODE_DESC& desc = m_ChainDesc.BufferDesc;
    selectResolution(desc.Width, desc.Height, bWindowed);
    if (bWindowed)
    {
        desc.RefreshRate.Numerator = 60;
        desc.RefreshRate.Denominator = 1;
    }
    else
        desc.RefreshRate = selectRefresh(desc.Width, desc.Height, desc.Format);
    CHK_DX(m_pSwapChain->ResizeTarget(&desc));

    _SHOW_REF("refCount:pBaseZB", pBaseZB);
    _SHOW_REF("refCount:pBaseRT", pBaseRT);
    _RELEASE(pBaseZB);
    _RELEASE(pBaseRT);
    CHK_DX(m_pSwapChain->ResizeBuffers(
        cd.BufferCount, desc.Width, desc.Height, desc.Format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    UpdateViews();

    updateWindowProps(hwnd);
    ShowWindow(hwnd, SW_SHOWNORMAL);
}

D3DFORMAT CHW::selectDepthStencil(D3DFORMAT /*fTarget*/)
{
// R3 hack
#pragma todo("R3 need to specify depth format")
    return D3DFMT_D24S8;
}

void CHW::selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
    fill_vid_mode_list(this);

    if (bWindowed)
    {
        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
    else // check
    {
        string64 buff;
        xr_sprintf(buff, sizeof(buff), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);

        if (_ParseItem(buff, GEnv.vid_mode_token) == u32(-1)) // not found
        { // select safe
            xr_sprintf(buff, sizeof(buff), "vid_mode %s", GEnv.vid_mode_token[0].name);
            Console->Execute(buff);
        }

        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
}

DXGI_RATIONAL CHW::selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt)
{
    if (psDeviceFlags.is(rsRefresh60hz))
    {
        return DXGI_RATIONAL({ 60, 1 });
    }
    else
    {
        xr_vector<DXGI_MODE_DESC> modes;

        IDXGIOutput* pOutput;
        m_pAdapter->EnumOutputs(0, &pOutput);
        VERIFY(pOutput);

        UINT num = 0;
        DXGI_FORMAT format = fmt;
        UINT flags = 0;

        // Get the number of display modes available
        pOutput->GetDisplayModeList(format, flags, &num, nullptr);

        // Get the list of display modes
        modes.resize(num);
        pOutput->GetDisplayModeList(format, flags, &num, &modes.front());

        _RELEASE(pOutput);

        float CurrentFreq = 60.0f;
        DXGI_RATIONAL res = { 60, 1 };

        for (auto &i : modes)
        {
            if ((i.Width == dwWidth) && (i.Height == dwHeight))
            {
                VERIFY(i.RefreshRate.Denominator);
                float TempFreq = float(i.RefreshRate.Numerator) / float(i.RefreshRate.Denominator);
                if (TempFreq > CurrentFreq)
                {
                    CurrentFreq = TempFreq;
                    res = i.RefreshRate;
                }
            }
        }

        return res;
    }
}

BOOL CHW::support(D3DFORMAT fmt, DWORD type, DWORD usage)
{
    // TODO: DX10: implement stub for this code.
    VERIFY(!"Implement CHW::support");
    return TRUE;
}

void CHW::updateWindowProps(HWND m_hWnd)
{
    if (strstr(Core.Params, "-wnd_mode"))
        psDeviceFlags.set(rsFullscreen, false);

    bool bWindowed = !psDeviceFlags.is(rsFullscreen);

    u32 dwWindowStyle = 0;
    // Set window properties depending on what mode were in.
    if (bWindowed)
    {
        if (m_move_window)
        {
            const bool drawBorders = strstr(Core.Params, "-draw_borders");
            dwWindowStyle = WS_VISIBLE;
            if (drawBorders)
                dwWindowStyle |= WS_BORDER | WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX;
            SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle);
            // When moving from fullscreen to windowed mode, it is important to
            // adjust the window size after recreating the device rather than
            // beforehand to ensure that you get the window size you want.  For
            // example, when switching from 640x480 fullscreen to windowed with
            // a 1000x600 window on a 1024x768 desktop, it is impossible to set
            // the window size to 1000x600 until after the display mode has
            // changed to 1024x768, because windows cannot be larger than the
            // desktop.

            RECT m_rcWindowBounds;
            float fYOffset = 0.f;
            bool centerScreen = false;
            if (strstr(Core.Params, "-center_screen"))
                centerScreen = true;

            if (centerScreen)
            {
                RECT DesktopRect;
                GetClientRect(GetDesktopWindow(), &DesktopRect);

                SetRect(&m_rcWindowBounds,
                    (DesktopRect.right - m_ChainDesc.BufferDesc.Width) / 2,
                    (DesktopRect.bottom - m_ChainDesc.BufferDesc.Height) / 2,
                    (DesktopRect.right + m_ChainDesc.BufferDesc.Width) / 2,
                    (DesktopRect.bottom + m_ChainDesc.BufferDesc.Height) / 2);
            }
            else
            {
                if (drawBorders)
                    fYOffset = GetSystemMetrics(SM_CYCAPTION); // size of the window title bar
                SetRect(&m_rcWindowBounds, 0, 0, m_ChainDesc.BufferDesc.Width, m_ChainDesc.BufferDesc.Height);
            };

            AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

            SetWindowPos(m_hWnd, HWND_NOTOPMOST,
                         m_rcWindowBounds.left, m_rcWindowBounds.top + fYOffset,
                         m_rcWindowBounds.right - m_rcWindowBounds.left,
                         m_rcWindowBounds.bottom - m_rcWindowBounds.top,
                         SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_DRAWFRAME);
        }
    }
    else
    {
        SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = WS_POPUP | WS_VISIBLE);
    }

    ShowCursor(FALSE);
    SetForegroundWindow(m_hWnd);
}

struct _uniq_mode
{
    _uniq_mode(LPCSTR v) : _val(v) {}
    LPCSTR _val;
    bool operator()(LPCSTR _other) { return !xr_stricmp(_val, _other); }
};

#ifndef _EDITOR

void free_vid_mode_list()
{
    for (int i = 0; GEnv.vid_mode_token[i].name; i++)
    {
        xr_free(GEnv.vid_mode_token[i].name);
    }
    xr_free(GEnv.vid_mode_token);
    GEnv.vid_mode_token = nullptr;
}

void fill_vid_mode_list(CHW* _hw)
{
    if (GEnv.vid_mode_token != nullptr)
        return;
    xr_vector<LPCSTR> _tmp;
    xr_vector<DXGI_MODE_DESC> modes;

    IDXGIOutput* pOutput;
    //_hw->m_pSwapChain->GetContainingOutput(&pOutput);
    _hw->m_pAdapter->EnumOutputs(0, &pOutput);
    VERIFY(pOutput);

    UINT cnt = 0;
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    UINT flags = 0;

    // Get the number of display modes available
    pOutput->GetDisplayModeList(format, flags, &cnt, nullptr);

    // Get the list of display modes
    modes.resize(cnt);
    pOutput->GetDisplayModeList(format, flags, &cnt, &modes.front());

    _RELEASE(pOutput);

    for (auto &i : modes)
    {
        string32 str;

        if (i.Width < 800)
            continue;

        xr_sprintf(str, sizeof(str), "%dx%d", i.Width, i.Height);

        if (_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
            continue;

        _tmp.push_back(nullptr);
        _tmp.back() = xr_strdup(str);
    }

    u32 _cnt = _tmp.size() + 1;

    GEnv.vid_mode_token = xr_alloc<xr_token>(_cnt);

    GEnv.vid_mode_token[_cnt - 1].id = -1;
    GEnv.vid_mode_token[_cnt - 1].name = nullptr;

#ifdef DEBUG
    Msg("Available video modes[%d]:", _tmp.size());
#endif // DEBUG
    for (auto i = 0; i < _tmp.size(); ++i)
    {
        GEnv.vid_mode_token[i].id = i;
        GEnv.vid_mode_token[i].name = _tmp[i];
#ifdef DEBUG
        Msg("[%s]", _tmp[i]);
#endif // DEBUG
    }
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
#endif
