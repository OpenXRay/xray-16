#include "stdafx.h"

#include "dx9HW.h"

ENGINE_API extern u32 Vid_SelectedRefreshRate;

CHW HW;

CHW::CHW() {}
CHW::~CHW() {}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
    hD3D = XRay::LoadModule(GEnv.isDedicatedServer ? "xrD3D9-Null" : "d3d9");
    R_ASSERT2(hD3D->IsLoaded(), "Can't find 'd3d9.dll'\nPlease install latest version of DirectX before running this program");

    using _Direct3DCreate9 = IDirect3D9* WINAPI(UINT SDKVersion);
    const auto createD3D = (_Direct3DCreate9*)hD3D->GetProcAddress("Direct3DCreate9");
    R_ASSERT(createD3D);
    pD3D = createD3D(D3D_SDK_VERSION);
    R_ASSERT2(pD3D, "Please install DirectX 9.0c");
}

void CHW::DestroyD3D()
{
    _SHOW_REF("refCount:pD3D", pD3D);
    _RELEASE(pD3D);
    hD3D = nullptr;
}

void CHW::CreateDevice(SDL_Window* m_sdlWnd)
{
    CreateD3D();

    const bool bWindowed = ThisInstanceIsGlobal() ? !psDeviceFlags.is(rsFullscreen) : true;

    m_DriverType = Caps.bForceGPU_REF ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;

    DevAdapter = D3DADAPTER_DEFAULT;

    // Display the name of video board
    D3DADAPTER_IDENTIFIER9 adapterID;
    R_CHK(pD3D->GetAdapterIdentifier(DevAdapter, 0, &adapterID));
    Msg("* GPU [vendor:%X]-[device:%X]: %s", adapterID.VendorId, adapterID.DeviceId, adapterID.Description);

    const u16 driverProduct    = HIWORD(adapterID.DriverVersion.HighPart);
    const u16 driverVersion    = LOWORD(adapterID.DriverVersion.HighPart);
    const u16 driverSubVersion = HIWORD(adapterID.DriverVersion.LowPart);
    const u16 driverBuild      = LOWORD(adapterID.DriverVersion.LowPart);
    Msg("* GPU driver: %d.%d.%d.%d", u32(driverProduct), u32(driverVersion), u32(driverSubVersion), u32(driverBuild));

    Caps.id_vendor = adapterID.VendorId;
    Caps.id_device = adapterID.DeviceId;

    // Select back-buffer & depth-stencil format
    D3DFORMAT& fTarget = Caps.fTarget;
    D3DFORMAT& fDepth = Caps.fDepth;
    if (bWindowed)
    {
        // Retrieve display mode
        D3DDISPLAYMODE mode;
        R_CHK(pD3D->GetAdapterDisplayMode(DevAdapter, &mode));

        // Apply its format
        fTarget = mode.Format;

        // Apply depth
        R_CHK(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, TRUE));
        fDepth = selectDepthStencil(fTarget);
    }
    else
    {
        switch (psCurrentBPP)
        {
        case 32:
            fTarget = D3DFMT_X8R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_A8R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_R8G8B8;
            if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_UNKNOWN;
            break;
        case 16:
        default:
            fTarget = D3DFMT_R5G6B5;
            if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_X1R5G5B5;
            if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_X4R4G4B4;
            if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter, m_DriverType, fTarget, fTarget, FALSE)))
                break;
            fTarget = D3DFMT_UNKNOWN;
            break;
        }
        fDepth = selectDepthStencil(fTarget);
    }

    if (D3DFMT_UNKNOWN == fTarget || D3DFMT_UNKNOWN == fDepth)
    {
        Log("Failed to initialize graphics hardware.\n"
            "Please try to restart the game.\n"
            "Can not find matching format for back buffer.");
        xrDebug::DoExit("Failed to initialize graphics hardware.\nPlease try to restart the game.");
    }

    // Set up the presentation parameters
    D3DPRESENT_PARAMETERS& P = DevPP;
    ZeroMemory(&P, sizeof(P));

    P.BackBufferWidth = Device.dwWidth;
    P.BackBufferHeight = Device.dwHeight;

    // Back buffer
    BackBufferCount = 1;
    P.BackBufferFormat = fTarget;
    P.BackBufferCount = BackBufferCount;

    // Multisample
    P.MultiSampleType = D3DMULTISAMPLE_NONE;
    P.MultiSampleQuality = 0;

    // Windoze
    P.SwapEffect = bWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(m_sdlWnd, &info))
    {
        switch (info.subsystem)
        {
        case SDL_SYSWM_WINDOWS:
            P.hDeviceWindow = info.info.win.window;
            break;
        default: break;
        }
    }
    else
        Log("! Couldn't get window information: ", SDL_GetError());

    P.Windowed = bWindowed;

    // Depth/stencil
    P.EnableAutoDepthStencil = TRUE;
    P.AutoDepthStencilFormat = fDepth;
    P.Flags = 0; //. D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

    // Refresh rate
    if (bWindowed)
    {
        P.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        P.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    }
    else
    {
        P.PresentationInterval = selectPresentInterval(); // Vsync (R1\R2)
        P.FullScreen_RefreshRateInHz = Vid_SelectedRefreshRate;
    }


    // Create the device
    const auto GPU = selectGPU();
    auto result = pD3D->CreateDevice(DevAdapter, m_DriverType, P.hDeviceWindow,
        GPU | D3DCREATE_MULTITHREADED, //. ? locks at present
        &P, &pDevice);

    if (FAILED(result))
    {
        result = pD3D->CreateDevice(DevAdapter, m_DriverType, P.hDeviceWindow,
            GPU | D3DCREATE_MULTITHREADED, //. ? locks at present
            &P, &pDevice);
    }
    if (D3DERR_DEVICELOST == result)
    {
        // Fatal error! Cannot create rendering device AT STARTUP !!!
        Msg("Failed to initialize graphics hardware.\n"
            "Please try to restart the game.\n"
            "CreateDevice returned 0x%08x(D3DERR_DEVICELOST)", result);
        xrDebug::DoExit("Failed to initialize graphics hardware.\nPlease try to restart the game.");
    };

    _SHOW_REF("* CREATE: DeviceREF:", pDevice);
    switch (GPU)
    {
    case D3DCREATE_SOFTWARE_VERTEXPROCESSING:                        Log("* Vertex Processor: SOFTWARE"     ); break;
    case D3DCREATE_MIXED_VERTEXPROCESSING:                           Log("* Vertex Processor: MIXED"        ); break;
    case D3DCREATE_HARDWARE_VERTEXPROCESSING:                        Log("* Vertex Processor: HARDWARE"     ); break;
    case D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE: Log("* Vertex Processor: PURE HARDWARE"); break;
    }

    // Capture PIX events
    d3dperf_BeginEvent = static_cast<decltype(d3dperf_BeginEvent)>(hD3D->GetProcAddress("D3DPERF_BeginEvent"));
    d3dperf_EndEvent = static_cast<decltype(d3dperf_EndEvent)>(hD3D->GetProcAddress("D3DPERF_EndEvent"));

    // Capture misc data
#ifdef DEBUG
    R_CHK(pDevice->CreateStateBlock(D3DSBT_ALL, &dwDebugSB));
#endif
    const u32 memory = pDevice->GetAvailableTextureMem();
    Msg("*   Texture memory: %d M", memory / (1024 * 1024));
    Msg("*        DDI-level: %2.1f", float(D3DXGetDriverLevel(pDevice)) / 100.f);
}

void CHW::DestroyDevice()
{
#ifdef DEBUG
    _SHOW_REF("refCount:dwDebugSB", dwDebugSB);
    _RELEASE(dwDebugSB);
#endif
    _SHOW_REF("DeviceREF:", pDevice);
    _RELEASE(pDevice);

    DestroyD3D();
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset()
{
#ifdef DEBUG
    _RELEASE(dwDebugSB);
#endif
    DevPP.BackBufferWidth = Device.dwWidth;
    DevPP.BackBufferHeight = Device.dwHeight;

    // Windoze
    const bool bWindowed = ThisInstanceIsGlobal() ? !psDeviceFlags.is(rsFullscreen) : true;
    DevPP.SwapEffect = bWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;
    DevPP.Windowed = bWindowed;
    if (!bWindowed)
    {
        DevPP.PresentationInterval = selectPresentInterval(); // Vsync (R1\R2)
        DevPP.FullScreen_RefreshRateInHz = Vid_SelectedRefreshRate;
    }
    else
    {
        DevPP.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        DevPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    }

    while (true)
    {
        const HRESULT result = pDevice->Reset(&DevPP);
        
        if (SUCCEEDED(result))
            break;

        Msg("! ERROR: [%dx%d]: %s", DevPP.BackBufferWidth, DevPP.BackBufferHeight, xrDebug::ErrorToString(result));
        Sleep(100);
    }
#ifdef DEBUG
    R_CHK(pDevice->CreateStateBlock(D3DSBT_ALL, &dwDebugSB));
#endif
}

D3DFORMAT CHW::selectDepthStencil(D3DFORMAT fTarget) const
{
    // R2 hack
#pragma todo("R2 need to specify depth format")
    if (GEnv.Render->GenerationIsR2())
        return D3DFMT_D24S8;

    // R1 usual
    constexpr D3DFORMAT formats[] =
    {
        D3DFMT_D24S8,
        D3DFMT_D24X4S4,
        D3DFMT_D32,
        D3DFMT_D24X8,
        D3DFMT_D16,
        D3DFMT_D15S1
    };

    for (D3DFORMAT fmt : formats)
    {
        if (SUCCEEDED(pD3D->CheckDeviceFormat(
            DevAdapter, m_DriverType, fTarget, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, fmt)))
        {
            if (SUCCEEDED(pD3D->CheckDepthStencilMatch(DevAdapter, m_DriverType, fTarget, fTarget, fmt)))
            {
                return fmt;
            }
        }
    }
    return D3DFMT_UNKNOWN;
}

bool CHW::ThisInstanceIsGlobal() const
{
    return this == &HW;
}

void CHW::BeginPixEvent(LPCWSTR wszName) const
{
    if (d3dperf_BeginEvent)
        d3dperf_BeginEvent(D3DCOLOR_RGBA(127, 0, 0, 255), wszName);
}

void CHW::EndPixEvent() const
{
    if (d3dperf_EndEvent)
        d3dperf_EndEvent();
}

u32 CHW::selectPresentInterval() const
{
    D3DCAPS9 caps;
    pD3D->GetDeviceCaps(DevAdapter, m_DriverType, &caps);

    if (!psDeviceFlags.test(rsVSync))
    {
        if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
            return D3DPRESENT_INTERVAL_IMMEDIATE;
        if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
            return D3DPRESENT_INTERVAL_ONE;
    }
    return D3DPRESENT_INTERVAL_DEFAULT;
}

bool CHW::GivenGPUIsIntelGMA(u32 id_vendor, u32 id_device)
{
    if (id_vendor == 0x8086) // Intel
    {
        constexpr u32 IntelGMA_SoftList[] =
        {
            0x2782, 0x2582, 0x2792, 0x2592, 0x2772, 0x2776, 0x27A2, 0x27A6, 0x27AE,
            0x2982, 0x2983, 0x2992, 0x2993, 0x29A2, 0x29A3, 0x2972, 0x2973, 0x2A02,
            0x2A03, 0x2A12, 0x2A13, 0x29C2, 0x29C3, 0x29B2, 0x29B3, 0x29D2, 0x29D3,
            0x2A42, 0x2A43, 0x2E02, 0x2E03, 0x2E12, 0x2E13, 0x2E22, 0x2E23, 0x2E32,
            0x2E33, 0x2E42, 0x2E43, 0x2E92, 0x2E93, 0x0042, 0x0046
        };

        for (u32 idx : IntelGMA_SoftList)
        {
            if (idx == id_device)
                return true;
        }
    }
    return false;
}

void AdjustSkinningMode(bool isIntelGMA)
{
    if (isIntelGMA)
    {
        switch (ps_r1_SoftwareSkinning)
        {
        case 0:
            Log("* Enabling software skinning");
            ps_r1_SoftwareSkinning = 1;
            break;
        case 1: Log("* Using software skinning"); break;
        case 2:
            Log("* WARNING: Using hardware skinning");
            Log("*   setting 'r1_software_skinning' to '1' may improve performance");
            break;
        }
    }
    else if (ps_r1_SoftwareSkinning == 1)
    {
        Msg("* WARNING: Using software skinning");
        Msg("*   setting 'r1_software_skinning' to '0' should improve performance");
    }
}

u32 CHW::selectGPU() const
{
    if (ThisInstanceIsGlobal())
        AdjustSkinningMode(GivenGPUIsIntelGMA(Caps.id_vendor, Caps.id_device));

    if (Caps.bForceGPU_SW)
        return D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    D3DCAPS9 caps;
    pD3D->GetDeviceCaps(DevAdapter, m_DriverType, &caps);

    if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
        if (Caps.bForceGPU_NonPure)
            return D3DCREATE_HARDWARE_VERTEXPROCESSING;
        if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
            return D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
        return D3DCREATE_HARDWARE_VERTEXPROCESSING;
        // return D3DCREATE_MIXED_VERTEXPROCESSING;
    }

    return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
}

BOOL CHW::support(D3DFORMAT fmt, u32 type, u32 usage) const
{
    const HRESULT result = pD3D->CheckDeviceFormat(DevAdapter, m_DriverType, Caps.fTarget, usage, (D3DRESOURCETYPE)type, fmt);
    if (FAILED(result))
        return FALSE;
    return TRUE;
}

std::pair<u32, u32> CHW::GetSurfaceSize() const
{
    return
    {
        DevPP.BackBufferWidth,
        DevPP.BackBufferHeight
    };
}

void CHW::BeginScene()
{
    CHK_DX(HW.pDevice->BeginScene());
}

void CHW::EndScene()
{
    CHK_DX(HW.pDevice->EndScene());
}

void CHW::Present()
{
    pDevice->Present(nullptr, nullptr, nullptr, nullptr);
    CurrentBackBuffer = (CurrentBackBuffer + 1) % BackBufferCount;
}

DeviceState CHW::GetDeviceState() const
{
    const auto result = pDevice->TestCooperativeLevel();

    switch (result)
    {
        // If the device was lost, do not render until we get it back
    case D3DERR_DEVICELOST:
        return DeviceState::Lost;

        // Check if the device is ready to be reset
    case D3DERR_DEVICENOTRESET:
        return DeviceState::NeedReset;
    }

    return DeviceState::Normal;
}
