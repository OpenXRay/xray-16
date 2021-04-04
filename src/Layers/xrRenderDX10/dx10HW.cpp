#include "stdafx.h"

#include "dx10HW.h"

#include "StateManager/dx10SamplerStateCache.h"
#include "StateManager/dx10StateCache.h"
#include "dx10TextureUtils.h"

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
        m_pSwapChain->SetFullscreenState(ThisInstanceIsGlobal() ? psDeviceFlags.is(rsFullscreen) : false, NULL);
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
    hDXGI = XRay::LoadModule("dxgi");
    hD3D = XRay::LoadModule("d3d11");
    if (!hD3D->IsLoaded() || !hDXGI->IsLoaded())
    {
        Valid = false;
        return;
    }

    // Минимально поддерживаемая версия Windows => Windows Vista SP2 или Windows 7.
    const auto createDXGIFactory = static_cast<decltype(&CreateDXGIFactory1)>(hDXGI->GetProcAddress("CreateDXGIFactory1"));
    R_CHK(createDXGIFactory(__uuidof(IDXGIFactory1), (void**)(&m_pFactory)));

    R_CHK(m_pFactory->EnumAdapters1(0, &m_pAdapter));
}

void CHW::DestroyD3D()
{
    _SHOW_REF("refCount:m_pAdapter", m_pAdapter);
    _RELEASE(m_pAdapter);

    _SHOW_REF("refCount:m_pFactory", m_pFactory);
    _RELEASE(m_pFactory);
}

void CHW::CreateDevice(SDL_Window* sdlWnd)
{
    CreateD3D();
    if (!Valid)
        return;

    m_DriverType = Caps.bForceGPU_REF ? D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

    // Display the name of video board
    DXGI_ADAPTER_DESC1 Desc;
    R_CHK(m_pAdapter->GetDesc1(&Desc));
    //  Warning: Desc.Description is wide string
    Msg("* GPU [vendor:%X]-[device:%X]: %S", Desc.VendorId, Desc.DeviceId, Desc.Description);

    Caps.id_vendor = Desc.VendorId;
    Caps.id_device = Desc.DeviceId;

    u32 createDeviceFlags = 0;

#ifdef DEBUG
    if (xrDebug::DebuggerIsPresent())
        createDeviceFlags |= D3D_CREATE_DEVICE_DEBUG;
#endif

    HRESULT R;

    D3D_FEATURE_LEVEL featureLevels[] =
    {
#ifdef HAS_DX11_3
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
#endif
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL featureLevels2[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL featureLevels3[] =
    {
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    const auto createDevice = [&](const D3D_FEATURE_LEVEL* level, const u32 levels)
    {
        static const auto d3d11CreateDevice = static_cast<PFN_D3D11_CREATE_DEVICE>(hD3D->GetProcAddress("D3D11CreateDevice"));
        return d3d11CreateDevice(m_pAdapter, D3D_DRIVER_TYPE_UNKNOWN,
            nullptr, createDeviceFlags, level, levels,
            D3D11_SDK_VERSION, &pDevice, &FeatureLevel, &pContext);
    };

    if (DX10Only)
        R = createDevice(featureLevels3, std::size(featureLevels3));
    else
    {
        R = createDevice(featureLevels, std::size(featureLevels));
        if (FAILED(R))
            R = createDevice(featureLevels2, std::size(featureLevels2));
    }

    if (SUCCEEDED(R))
    {
        pContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&pContext1));
        pContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void**>(&pAnnotation));
#ifdef HAS_DX11_3
        pDevice->QueryInterface(__uuidof(ID3D11Device3), reinterpret_cast<void**>(&pDevice3));
#endif
        if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            D3DCompile = &::D3DCompile;
            ComputeShadersSupported = true;
        }
        else
        {
            if (ClearSkyMode)
            {
                hD3DCompiler = XRay::LoadModule("d3dcompiler_37");
                D3DCompile = static_cast<D3DCompileFunc>(hD3DCompiler->GetProcAddress("D3DCompileFromMemory"));
            }
            else
            {
                D3DCompile = &::D3DCompile;
            }

            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS data;
            pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS,
                &data, sizeof(data));
            ComputeShadersSupported = data.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x;
        }
        D3D11_FEATURE_DATA_D3D11_OPTIONS options;
        pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(options));

        D3D11_FEATURE_DATA_DOUBLES doubles;
        pDevice->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &doubles, sizeof(doubles));

        DoublePrecisionFloatShaderOps = doubles.DoublePrecisionFloatShaderOps;
        SAD4ShaderInstructions = options.SAD4ShaderInstructions;
        ExtendedDoublesShaderInstructions = options.ExtendedDoublesShaderInstructions;
    }

    if (FAILED(R))
    {
        Valid = false;
        if (!ThisInstanceIsGlobal())
            return;
        // Fatal error! Cannot create rendering device AT STARTUP !!!
        Msg("Failed to initialize graphics hardware.\n"
            "Please try to restart the game.\n"
            "CreateDevice returned 0x%08x", R);
        xrDebug::DoExit("Failed to initialize graphics hardware.\nPlease try to restart the game.");
    }

    _SHOW_REF("* CREATE: DeviceREF:", pDevice);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);

    R_ASSERT2(SDL_GetWindowWMInfo(sdlWnd, &info), SDL_GetError());

    const HWND hwnd = info.info.win.window;

    if (!CreateSwapChain2(hwnd))
    {
        CreateSwapChain(hwnd);
    }

    // Select depth-stencil format
    constexpr DXGI_FORMAT formats[] =
    {
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_D16_UNORM
    };
    const DXGI_FORMAT selectedFormat = SelectFormat(D3D_FORMAT_SUPPORT_DEPTH_STENCIL, formats);
    if (selectedFormat == DXGI_FORMAT_UNKNOWN)
    {
        Valid = false;
        if (!ThisInstanceIsGlobal())
            return;
        Log("Failed to initialize graphics hardware: "
            "failed to select depth-stencil format.\n"
            "Please try to restart the game.");
        xrDebug::DoExit("Failed to initialize graphics hardware.\nPlease try to restart the game.");
    }
    Caps.fDepth = dx10TextureUtils::ConvertTextureFormat(selectedFormat);

    const auto memory = Desc.DedicatedVideoMemory;
    Msg("*   Texture memory: %d M", memory / (1024 * 1024));
    //Msg("*        DDI-level: %2.1f", float(D3DXGetDriverLevel(pDevice)) / 100.f);
}

void CHW::CreateSwapChain(HWND hwnd)
{
    // Set up the presentation parameters
    DXGI_SWAP_CHAIN_DESC& sd = m_ChainDesc;
    ZeroMemory(&sd, sizeof(sd));

    // Back buffer
    sd.BufferDesc.Width = Device.dwWidth;
    sd.BufferDesc.Height = Device.dwHeight;

    //  TODO: DX10: implement dynamic format selection
    constexpr DXGI_FORMAT formats[] =
    {
        //DXGI_FORMAT_R16G16B16A16_FLOAT, // Do we even need this?
        //DXGI_FORMAT_R10G10B10A2_UNORM, // D3DX11SaveTextureToMemory fails on this format
        DXGI_FORMAT_R8G8B8A8_UNORM,
    };

    // Select back-buffer format
    sd.BufferDesc.Format = SelectFormat(D3D_FORMAT_SUPPORT_DISPLAY, formats);
    Caps.fTarget = dx10TextureUtils::ConvertTextureFormat(sd.BufferDesc.Format);

    // Buffering
    BackBufferCount = 1;
    sd.BufferCount = BackBufferCount;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

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

    sd.OutputWindow = hwnd;

    sd.Windowed = ThisInstanceIsGlobal() ? !psDeviceFlags.is(rsFullscreen) : true;

    //  Additional set up
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    R_CHK(m_pFactory->CreateSwapChain(pDevice, &sd, &m_pSwapChain));
}

bool CHW::CreateSwapChain2(HWND hwnd)
{
    if (strstr(Core.Params, "-no_dx11_2"))
        return false;

#ifdef HAS_DX11_2
    IDXGIFactory2* pFactory2{};
    m_pAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pFactory2);
    if (!pFactory2)
        return false;

    // Set up the presentation parameters
    DXGI_SWAP_CHAIN_DESC1 desc{};

    // Back buffer
    desc.Width = Device.dwWidth;
    desc.Height = Device.dwHeight;

    constexpr DXGI_FORMAT formats[] =
    {
        //DXGI_FORMAT_R16G16B16A16_FLOAT,
        //DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM,
    };

    // Select back-buffer format
    desc.Format = SelectFormat(D3D11_FORMAT_SUPPORT_DISPLAY, formats);
    Caps.fTarget = dx10TextureUtils::ConvertTextureFormat(desc.Format);

    // Buffering
    BackBufferCount = 1; // For DXGI_SWAP_EFFECT_FLIP_DISCARD we need at least two
    desc.BufferCount = BackBufferCount;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    // Multisample
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    // Windoze
    //desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // XXX: tearing glitches with flip presentation model
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Scaling = DXGI_SCALING_STRETCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fulldesc{};
    fulldesc.Windowed = ThisInstanceIsGlobal() ? !psDeviceFlags.is(rsFullscreen) : true;

    // Additional setup
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    IDXGISwapChain1* swapchain{};
    const HRESULT result = pFactory2->CreateSwapChainForHwnd(pDevice, hwnd, &desc,
        fulldesc.Windowed ? nullptr : &fulldesc, nullptr, &swapchain);
    _RELEASE(pFactory2);

    if (FAILED(result))
        return false;

    m_pSwapChain = swapchain;
    R_CHK(m_pSwapChain->GetDesc(&m_ChainDesc));

    m_pSwapChain->QueryInterface(__uuidof(IDXGISwapChain2), reinterpret_cast<void**>(&m_pSwapChain2));

    if (m_pSwapChain2 && ThisInstanceIsGlobal())
        Device.PresentationFinished = m_pSwapChain2->GetFrameLatencyWaitableObject();

    return true;
#else // #ifdef HAS_DX11_2
    UNUSED(hwnd);
#endif

    return false;
}

bool CHW::ThisInstanceIsGlobal() const
{
    return this == &HW;
}

void CHW::BeginPixEvent(LPCWSTR wszName) const
{
    if (pAnnotation)
        pAnnotation->BeginEvent(wszName);
}

void CHW::EndPixEvent() const
{
    if (pAnnotation)
        pAnnotation->EndEvent();
}

void CHW::DestroyDevice()
{
    //  Destroy state managers
    if (ThisInstanceIsGlobal()) // only if we are global HW
    {
        StateManager.Reset();
        RSManager.ClearStateArray();
        DSSManager.ClearStateArray();
        BSManager.ClearStateArray();
        SSManager.ClearStateArray();
    }
    //  Must switch to windowed mode to release swap chain
    if (!m_ChainDesc.Windowed)
        m_pSwapChain->SetFullscreenState(FALSE, NULL);
    _SHOW_REF("refCount:m_pSwapChain", m_pSwapChain);
    _RELEASE(m_pSwapChain);
#ifdef HAS_DX11_2
    _SHOW_REF("refCount:m_pSwapChain2", m_pSwapChain2);
    _RELEASE(m_pSwapChain2);
#endif

    _RELEASE(pContext1);
    _RELEASE(pContext);

    _SHOW_REF("refCount:pDevice:", pDevice);
    _RELEASE(pDevice);

#ifdef HAS_DX11_3
    _SHOW_REF("refCount:pDevice3:", pDevice3);
    _RELEASE(pDevice3);
#endif

    DestroyD3D();
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset()
{
    DXGI_SWAP_CHAIN_DESC& cd = m_ChainDesc;
    const bool bWindowed = ThisInstanceIsGlobal() ? !psDeviceFlags.is(rsFullscreen) : true;
    cd.Windowed = bWindowed;
    m_pSwapChain->SetFullscreenState(!bWindowed, NULL);
    DXGI_MODE_DESC& desc = m_ChainDesc.BufferDesc;
    desc.Width = Device.dwWidth;
    desc.Height = Device.dwHeight;

    CHK_DX(m_pSwapChain->ResizeTarget(&desc));
    CHK_DX(m_pSwapChain->ResizeBuffers(
        cd.BufferCount, desc.Width, desc.Height, desc.Format, cd.Flags));
}

bool CHW::CheckFormatSupport(const DXGI_FORMAT format, const u32 feature) const
{
    u32 supports;

    if (SUCCEEDED(pDevice->CheckFormatSupport(format, &supports)))
    {
        if (supports & feature)
            return true;
    }

    return false;
}

DXGI_FORMAT CHW::SelectFormat(D3D_FORMAT_SUPPORT feature, const DXGI_FORMAT formats[], size_t count) const
{
    for (size_t i = 0; i < count; ++i)
        if (CheckFormatSupport(formats[i], feature))
            return formats[i];

    return DXGI_FORMAT_UNKNOWN;
}

bool CHW::UsingFlipPresentationModel() const
{
    return m_ChainDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
#ifdef HAS_DXGI1_4
        || m_ChainDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD
#endif
    ;
}

std::pair<u32, u32> CHW::GetSurfaceSize() const
{
    return
    {
        m_ChainDesc.BufferDesc.Width,
        m_ChainDesc.BufferDesc.Height
    };
}

void CHW::BeginScene() { }
void CHW::EndScene() { }

void CHW::Present()
{
    const bool bUseVSync = psDeviceFlags.is(rsFullscreen) &&
        psDeviceFlags.test(rsVSync); // xxx: weird tearing glitches when VSync turned on for windowed mode in DX10\11
    m_pSwapChain->Present(bUseVSync ? 1 : 0, 0);
#ifdef HAS_DX11_2
    if (m_pSwapChain2 && UsingFlipPresentationModel())
    {
        const float fps = Device.GetStats().fFPS;
        if (fps < 30)
            m_pSwapChain2->SetSourceSize(UINT(Device.dwWidth * 0.85f), UINT(Device.dwHeight * 0.85f));
        else if (fps < 15)
            m_pSwapChain2->SetSourceSize(UINT(Device.dwWidth * 0.7f), UINT(Device.dwHeight * 0.7f));
    }
#endif
    CurrentBackBuffer = (CurrentBackBuffer + 1) % BackBufferCount;
}

DeviceState CHW::GetDeviceState() const
{
    const auto result = m_pSwapChain->Present(0, DXGI_PRESENT_TEST);

    switch (result)
    {
        // Check if the device is ready to be reset
    case DXGI_ERROR_DEVICE_RESET:
        return DeviceState::NeedReset;
    }

    return DeviceState::Normal;
}
