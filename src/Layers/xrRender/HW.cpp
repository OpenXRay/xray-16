#include "stdafx.h"

#include "Layers/xrRender/HW.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrCore/xr_token.h"

extern ENGINE_API xr_vector<xr_token> AvailableVideoModes;

void fill_vid_mode_list(CHW* _hw);
void free_vid_mode_list();

CHW HW;

CHW::CHW() {}
CHW::~CHW() {}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
    hD3D = XRay::LoadModule(GEnv.isDedicatedServer ? "xrD3D9-Null" : "d3d9.dll");
    R_ASSERT2(hD3D->IsLoaded(), "Can't find 'd3d9.dll'\nPlease install latest version of DirectX before running this program");

    using _Direct3DCreate9 = IDirect3D9* WINAPI(UINT SDKVersion);
    auto createD3D = (_Direct3DCreate9*)hD3D->GetProcAddress("Direct3DCreate9");
    R_ASSERT(createD3D);
    pD3D = createD3D(D3D_SDK_VERSION);
    R_ASSERT2(pD3D, "Please install DirectX 9.0c");
}

void CHW::DestroyD3D()
{
    _SHOW_REF("refCount:pD3D", pD3D);
    _RELEASE(pD3D);
}

void CHW::CreateDevice(HWND m_hWnd, bool move_window)
{
    m_move_window = move_window;
    CreateD3D();

    bool bWindowed = !psDeviceFlags.is(rsFullscreen);
    if (GEnv.isDedicatedServer)
        bWindowed = true;

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

    // Retreive windowed mode
    D3DDISPLAYMODE mWindowed;
    R_CHK(pD3D->GetAdapterDisplayMode(DevAdapter, &mWindowed));

    // Select back-buffer & depth-stencil format
    D3DFORMAT& fTarget = Caps.fTarget;
    D3DFORMAT& fDepth = Caps.fDepth;
    if (bWindowed)
    {
        fTarget = mWindowed.Format;
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

    if ((D3DFMT_UNKNOWN == fTarget) || (D3DFMT_UNKNOWN == fTarget))
    {
        Msg("Failed to initialize graphics hardware.\n"
            "Please try to restart the game.\n"
            "Can not find matching format for back buffer.");
        FlushLog();
        MessageBox(nullptr, "Failed to initialize graphics hardware.\nPlease try to restart the game.", "Error!",
            MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    // Set up the presentation parameters
    D3DPRESENT_PARAMETERS& P = DevPP;
    ZeroMemory(&P, sizeof(P));

    selectResolution(P.BackBufferWidth, P.BackBufferHeight, bWindowed);

    // Back buffer
    P.BackBufferFormat = fTarget;
    P.BackBufferCount = 1;

    // Multisample
    P.MultiSampleType = D3DMULTISAMPLE_NONE;
    P.MultiSampleQuality = 0;

    // Windoze
    P.SwapEffect = bWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;
    P.hDeviceWindow = m_hWnd;
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
        P.FullScreen_RefreshRateInHz = selectRefresh(P.BackBufferWidth, P.BackBufferHeight, fTarget);
    }

    // Create the device
    const auto GPU = selectGPU();
    auto result = HW.pD3D->CreateDevice(DevAdapter, m_DriverType, m_hWnd,
        GPU | D3DCREATE_MULTITHREADED, //. ? locks at present
        &P, &pDevice);

    if (FAILED(result))
    {
        result = HW.pD3D->CreateDevice(DevAdapter, m_DriverType, m_hWnd,
            GPU | D3DCREATE_MULTITHREADED, //. ? locks at present
            &P, &pDevice);
    }
    if (D3DERR_DEVICELOST == result)
    {
        // Fatal error! Cannot create rendering device AT STARTUP !!!
        Msg("Failed to initialize graphics hardware.\n"
            "Please try to restart the game.\n"
            "CreateDevice returned 0x%08x(D3DERR_DEVICELOST)", result);
        FlushLog();
        MessageBox(nullptr, "Failed to initialize graphics hardware.\nPlease try to restart the game.", "Error!",
            MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
    };

    _SHOW_REF("* CREATE: DeviceREF:", HW.pDevice);
    switch (GPU)
    {
    case D3DCREATE_SOFTWARE_VERTEXPROCESSING:                        Log("* Vertex Processor: SOFTWARE"     ); break;
    case D3DCREATE_MIXED_VERTEXPROCESSING:                           Log("* Vertex Processor: MIXED"        ); break;
    case D3DCREATE_HARDWARE_VERTEXPROCESSING:                        Log("* Vertex Processor: HARDWARE"     ); break;
    case D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE: Log("* Vertex Processor: PURE HARDWARE"); break;
    }

// Capture misc data
#ifdef DEBUG
    R_CHK(pDevice->CreateStateBlock(D3DSBT_ALL, &dwDebugSB));
#endif
    R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
    R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));
    u32 memory = pDevice->GetAvailableTextureMem();
    Msg("*   Texture memory: %d M", memory / (1024 * 1024));
    Msg("*        DDI-level: %2.1f", float(D3DXGetDriverLevel(pDevice)) / 100.f);

    updateWindowProps(m_hWnd);
    fill_vid_mode_list(this);
}

void CHW::DestroyDevice()
{
    _SHOW_REF("refCount:pBaseZB", pBaseZB);
    _RELEASE(pBaseZB);

    _SHOW_REF("refCount:pBaseRT", pBaseRT);
    _RELEASE(pBaseRT);
#ifdef DEBUG
    _SHOW_REF("refCount:dwDebugSB", dwDebugSB);
    _RELEASE(dwDebugSB);
#endif
    _SHOW_REF("DeviceREF:", HW.pDevice);
    _RELEASE(HW.pDevice);

    DestroyD3D();

    free_vid_mode_list();
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset(HWND hwnd)
{
#ifdef DEBUG
    _RELEASE(dwDebugSB);
#endif
    _SHOW_REF("refCount:pBaseZB", pBaseZB);
    _SHOW_REF("refCount:pBaseRT", pBaseRT);
    _RELEASE(pBaseZB);
    _RELEASE(pBaseRT);

    bool bWindowed = true;
    if (!GEnv.isDedicatedServer)
        bWindowed = !psDeviceFlags.is(rsFullscreen);

    selectResolution(DevPP.BackBufferWidth, DevPP.BackBufferHeight, bWindowed);
    // Windoze
    DevPP.SwapEffect = bWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;
    DevPP.Windowed = bWindowed;
    if (!bWindowed)
    {
        DevPP.PresentationInterval = selectPresentInterval(); // Vsync (R1\R2)
        DevPP.FullScreen_RefreshRateInHz = selectRefresh(DevPP.BackBufferWidth, DevPP.BackBufferHeight, Caps.fTarget);
    }
    else
    {
        DevPP.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        DevPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    }

    while (true)
    {
        auto result = HW.pDevice->Reset(&DevPP);
        
        if (SUCCEEDED(result))
            break;

        Msg("! ERROR: [%dx%d]: %s", DevPP.BackBufferWidth, DevPP.BackBufferHeight, xrDebug::ErrorToString(result));
        Sleep(100);
    }
    R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
    R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));
#ifdef DEBUG
    R_CHK(pDevice->CreateStateBlock(D3DSBT_ALL, &dwDebugSB));
#endif

    updateWindowProps(hwnd);
    ShowWindow(hwnd, SW_SHOWNORMAL);
}

D3DFORMAT CHW::selectDepthStencil(D3DFORMAT fTarget)
{
    // R2 hack
#pragma todo("R2 need to specify depth format")
    if (GEnv.CurrentRenderer == 2)
        return D3DFMT_D24S8;

    // R1 usual
    static D3DFORMAT fDS_Try1[6] = { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D16, D3DFMT_D15S1 };

    D3DFORMAT* fDS_Try = fDS_Try1;
    int fDS_Cnt = 6;

    for (int it = 0; it < fDS_Cnt; it++)
    {
        if (SUCCEEDED(pD3D->CheckDeviceFormat(
            DevAdapter, m_DriverType, fTarget, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, fDS_Try[it])))
        {
            if (SUCCEEDED(pD3D->CheckDepthStencilMatch(DevAdapter, m_DriverType, fTarget, fTarget, fDS_Try[it])))
            {
                return fDS_Try[it];
            }
        }
    }
    return D3DFMT_UNKNOWN;
}

void CHW::selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
    fill_vid_mode_list(this);

    if (GEnv.isDedicatedServer)
    {
        dwWidth = 640;
        dwHeight = 480;
        return;
    }

    if (bWindowed)
    {
        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
    else // check
    {
        string64 buff;
        xr_sprintf(buff, sizeof(buff), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);

        if (_ParseItem(buff, AvailableVideoModes.data()) == u32(-1)) // not found
        { // select safe
            xr_sprintf(buff, sizeof(buff), "vid_mode %s", AvailableVideoModes[0].name);
            Console->Execute(buff);
        }

        dwWidth = psCurrentVidMode[0];
        dwHeight = psCurrentVidMode[1];
    }
}

u32 CHW::selectPresentInterval()
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

void CheckForIntelGMA(CHWCaps& Caps)
{
    bool isIntelGMA = false;

    if (Caps.id_vendor == 0x8086) // Intel
    {
        constexpr auto GMA_SL_SIZE = 43;
        constexpr DWORD IntelGMA_SoftList[GMA_SL_SIZE] =
        {
            0x2782, 0x2582, 0x2792, 0x2592, 0x2772, 0x2776, 0x27A2, 0x27A6, 0x27AE,
            0x2982, 0x2983, 0x2992, 0x2993, 0x29A2, 0x29A3, 0x2972, 0x2973, 0x2A02,
            0x2A03, 0x2A12, 0x2A13, 0x29C2, 0x29C3, 0x29B2, 0x29B3, 0x29D2, 0x29D3,
            0x2A42, 0x2A43, 0x2E02, 0x2E03, 0x2E12, 0x2E13, 0x2E22, 0x2E23, 0x2E32,
            0x2E33, 0x2E42, 0x2E43, 0x2E92, 0x2E93, 0x0042, 0x0046
        };

        for (int idx = 0; idx < GMA_SL_SIZE; ++idx)
        {
            if (IntelGMA_SoftList[idx] == Caps.id_device)
            {
                isIntelGMA = true;
                break;
            }
        }
    }

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

u32 CHW::selectGPU()
{
#if RENDER == R_R1
    CheckForIntelGMA(Caps);
#endif

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

u32 CHW::selectRefresh(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt)
{
    if (psDeviceFlags.is(rsRefresh60hz))
        return D3DPRESENT_RATE_DEFAULT;

    auto selected = D3DPRESENT_RATE_DEFAULT;
    const auto count = pD3D->GetAdapterModeCount(DevAdapter, fmt);
    for (u32 I = 0; I < count; I++)
    {
        D3DDISPLAYMODE Mode;
        pD3D->EnumAdapterModes(DevAdapter, fmt, I, &Mode);
        if (Mode.Width == dwWidth && Mode.Height == dwHeight)
        {
            //if (Mode.RefreshRate > selected)
            //    selected = Mode.RefreshRate;
            if (Mode.RefreshRate <= maxRefreshRate && Mode.RefreshRate>selected)
                selected = Mode.RefreshRate;  //ECO_RENDER modif.
        }
    }
    return selected;
}

BOOL CHW::support(D3DFORMAT fmt, DWORD type, DWORD usage)
{
    auto result = pD3D->CheckDeviceFormat(DevAdapter, m_DriverType, Caps.fTarget, usage, (D3DRESOURCETYPE)type, fmt);
    if (FAILED(result))
        return FALSE;
    return TRUE;
}

void CHW::updateWindowProps(HWND m_hWnd)
{
    bool bWindowed = !psDeviceFlags.is(rsFullscreen);

    if (GEnv.isDedicatedServer)
        bWindowed = true;

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
            if (GEnv.isDedicatedServer || strstr(Core.Params, "-center_screen"))
                centerScreen = true;

            if (centerScreen)
            {
                RECT DesktopRect;

                GetClientRect(GetDesktopWindow(), &DesktopRect);

                SetRect(&m_rcWindowBounds,
                    (DesktopRect.right - DevPP.BackBufferWidth) / 2,
                    (DesktopRect.bottom - DevPP.BackBufferHeight) / 2,
                    (DesktopRect.right + DevPP.BackBufferWidth) / 2,
                    (DesktopRect.bottom + DevPP.BackBufferHeight) / 2);
            }
            else
            {
                if (drawBorders)
                    fYOffset = GetSystemMetrics(SM_CYCAPTION); // size of the window title bar
                SetRect(&m_rcWindowBounds, 0, 0, DevPP.BackBufferWidth, DevPP.BackBufferHeight);
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

    if (!GEnv.isDedicatedServer)
        SetForegroundWindow(m_hWnd);
}

struct uniqueRenderingMode
{
    uniqueRenderingMode(pcstr v) : value(v) {}
    pcstr value;
    bool operator()(const xr_token other) const { return !xr_stricmp(value, other.name);}
};

void free_vid_mode_list()
{
    for (auto& mode : AvailableVideoModes)
        xr_free(mode.name);
    AvailableVideoModes.clear();
}

void fill_vid_mode_list(CHW* _hw)
{
    if (!AvailableVideoModes.empty())
        return;

    xr_vector<D3DDISPLAYMODE> displayModes;

    // Get the number of display modes available
    const auto cnt = _hw->pD3D->GetAdapterModeCount(_hw->DevAdapter, _hw->Caps.fTarget);

    // Get the list of display modes
    displayModes.resize(cnt);
    for (auto i = 0; i < cnt; ++i)
        _hw->pD3D->EnumAdapterModes(_hw->DevAdapter, _hw->Caps.fTarget, i, &displayModes[i]);

    int i = 0;
    auto& AVM = AvailableVideoModes;
    for (const auto& it : displayModes)
    {
        string32 str;

        xr_sprintf(str, sizeof(str), "%dx%d", it.Width, it.Height);

        if (AVM.cend() != std::find_if(AVM.cbegin(), AVM.cend(), uniqueRenderingMode(str)))
            continue;

        AVM.emplace_back(xr_token(xr_strdup(str), i));
        ++i;
    }
    AVM.emplace_back(xr_token(nullptr, -1));

    Msg("Available video modes[%d]:", AVM.size());
    for (const auto& mode : AVM)
        Msg("[%s]", mode.name);
}
