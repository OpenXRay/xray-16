#include "stdafx.h"
#include "xr_3da/resource.h"

#include "Include/editor/ide.hpp"
#include "engine_impl.hpp"
#include "GameFont.h"
#include "PerformanceAlert.hpp"
#include "xrCore/ModuleLookup.hpp"

extern LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void CRenderDevice::initialize_weather_editor()
{
    m_editor_module = XRay::LoadModule("xrWeatherEditor");
    if (!m_editor_module->IsLoaded())
        return;

    m_editor_initialize = (initialize_function_ptr)m_editor_module->GetProcAddress("initialize");
    VERIFY(m_editor_initialize);

    m_editor_finalize = (finalize_function_ptr)m_editor_module->GetProcAddress("finalize");
    VERIFY(m_editor_finalize);

    m_engine = new engine_impl();
    m_editor_initialize(m_editor, m_engine);
    VERIFY(m_editor);

    m_hWnd = m_editor->view_handle();
    VERIFY(m_hWnd != INVALID_HANDLE_VALUE);

    GEnv.isEditor = true;
}

void CRenderDevice::Initialize()
{
    Log("Initializing Engine...");
    TimerGlobal.Start();
    TimerMM.Start();

    if (strstr(Core.Params, "-weather"))
        initialize_weather_editor();

    // Unless a substitute hWnd has been specified, create a window to render into
    if (!m_hWnd)
    {
        const char* wndclass = "_XRAY_1.6";
        // Register the windows class
        HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
        WNDCLASS wndClass = {0, WndProc, 0, 0, hInstance, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
            LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), NULL, wndclass};
        RegisterClass(&wndClass);
        // Set the window's initial style
        m_dwWindowStyle = WS_BORDER | WS_DLGFRAME;
        // Set the window's initial width
        RECT rc;
        SetRect(&rc, 0, 0, 640, 480);
        AdjustWindowRect(&rc, m_dwWindowStyle, FALSE);
        // Create the render window
        m_hWnd = CreateWindowEx(WS_EX_TOPMOST, wndclass, "S.T.A.L.K.E.R.: Call of Pripyat", m_dwWindowStyle,
            CW_USEDEFAULT, CW_USEDEFAULT, (rc.right - rc.left), (rc.bottom - rc.top), 0L, 0, hInstance, 0L);
    }
    // Save window properties
    m_dwWindowStyle = GetWindowLong(m_hWnd, GWL_STYLE);
    GetWindowRect(m_hWnd, &m_rcWindowBounds);
    GetClientRect(m_hWnd, &m_rcWindowClient);
}

void CRenderDevice::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    font.OutNext("*** ENGINE:   %2.2fms", stats.EngineTotal.result);
    font.OutNext("FPS/RFPS:     %3.1f/%3.1f", stats.fFPS, stats.fRFPS);
    font.OutNext("TPS:          %2.2f M", stats.fTPS);
    if (alert && stats.fFPS < 30)
        alert->Print(font, "FPS       < 30:   %3.1f", stats.fFPS);
}
