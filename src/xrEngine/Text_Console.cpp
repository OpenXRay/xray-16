#include "stdafx.h"
#include "Text_Console.h"
#include "line_editor.h"
#include "SDL_syswm.h"

extern char const* const ioc_prompt;
extern char const* const ch_cursor;
int g_svTextConsoleUpdateRate = 1;

CTextConsole::CTextConsole()
{
    m_pMainWnd = NULL;
    m_hConsoleWnd = NULL;
    m_hLogWnd = NULL;
    m_hLogWndFont = NULL;

    m_bScrollLog = true;
    m_dwStartLine = 0;

    m_bNeedUpdate = false;
    m_dwLastUpdateTime = Device.dwTimeGlobal;
    m_last_time = Device.dwTimeGlobal;
}

CTextConsole::~CTextConsole() { m_pMainWnd = NULL; }
//-------------------------------------------------------------------------------------------
LRESULT CALLBACK TextConsole_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CTextConsole::CreateConsoleWnd()
{
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
    //----------------------------------
    RECT cRc;
    GetClientRect(*m_pMainWnd, &cRc);
    INT lX = cRc.left;
    INT lY = cRc.top;
    INT lWidth = cRc.right - cRc.left;
    INT lHeight = cRc.bottom - cRc.top;
    //----------------------------------
    const char* wndclass = "TEXT_CONSOLE";

    // Register the windows class
    WNDCLASS wndClass = {0, TextConsole_WndProc, 0, 0, hInstance, NULL, LoadCursor(hInstance, IDC_ARROW),
        GetStockBrush(GRAY_BRUSH), NULL, wndclass};
    RegisterClass(&wndClass);

    // Set the window's initial style
    u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE; // | WS_CLIPSIBLINGS;// | WS_CLIPCHILDREN;

    // Set the window's initial width
    RECT rc;
    SetRect(&rc, lX, lY, lWidth, lHeight);
    // AdjustWindowRect( &rc, dwWindowStyle, FALSE );

    // Create the render window
    m_hConsoleWnd = CreateWindow(
        wndclass, "XRAY Text Console", dwWindowStyle, lX, lY, lWidth, lHeight, *m_pMainWnd, 0, hInstance, 0L);
    //---------------------------------------------------------------------------
    R_ASSERT2(m_hConsoleWnd, "Unable to Create TextConsole Window!");
};
//-------------------------------------------------------------------------------------------
LRESULT CALLBACK TextConsole_LogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CTextConsole::CreateLogWnd()
{
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
    //----------------------------------
    RECT cRc;
    GetClientRect(m_hConsoleWnd, &cRc);
    INT lX = cRc.left;
    INT lY = cRc.top;
    INT lWidth = cRc.right - cRc.left;
    INT lHeight = cRc.bottom - cRc.top;
    //----------------------------------
    const char* wndclass = "TEXT_CONSOLE_LOG_WND";

    // Register the windows class
    WNDCLASS wndClass = {0, TextConsole_LogWndProc, 0, 0, hInstance, NULL, LoadCursor(NULL, IDC_ARROW),
        GetStockBrush(BLACK_BRUSH), NULL, wndclass};
    RegisterClass(&wndClass);

    // Set the window's initial style
    u32 dwWindowStyle = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE; // | WS_CLIPSIBLINGS;
    // u32 dwWindowStyleEx = WS_EX_CLIENTEDGE;

    // Set the window's initial width
    RECT rc;
    SetRect(&rc, lX, lY, lWidth, lHeight);
    // AdjustWindowRect( &rc, dwWindowStyle, FALSE );

    // Create the render window
    m_hLogWnd = CreateWindow(
        wndclass, "XRAY Text Console Log", dwWindowStyle, lX, lY, lWidth, lHeight, m_hConsoleWnd, 0, hInstance, 0L);
    //---------------------------------------------------------------------------
    R_ASSERT2(m_hLogWnd, "Unable to Create TextConsole Window!");
    //---------------------------------------------------------------------------
    ShowWindow(m_hLogWnd, SW_SHOW);
    UpdateWindow(m_hLogWnd);
    //-----------------------------------------------
    LOGFONT lf;
    lf.lfHeight = -12;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfClipPrecision = CLIP_STROKE_PRECIS;
    lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
    lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    xr_sprintf(lf.lfFaceName, sizeof(lf.lfFaceName), "");

    m_hLogWndFont = CreateFontIndirect(&lf);
    R_ASSERT2(m_hLogWndFont, "Unable to Create Font for Log Window");
    //------------------------------------------------
    m_hDC_LogWnd = GetDC(m_hLogWnd);
    R_ASSERT2(m_hDC_LogWnd, "Unable to Get DC for Log Window!");
    //------------------------------------------------
    m_hDC_LogWnd_BackBuffer = CreateCompatibleDC(m_hDC_LogWnd);
    R_ASSERT2(m_hDC_LogWnd_BackBuffer, "Unable to Create Compatible DC for Log Window!");
    //------------------------------------------------
    GetClientRect(m_hLogWnd, &cRc);
    lWidth = cRc.right - cRc.left;
    lHeight = cRc.bottom - cRc.top;
    //----------------------------------
    m_hBB_BM = CreateCompatibleBitmap(m_hDC_LogWnd, lWidth, lHeight);
    R_ASSERT2(m_hBB_BM, "Unable to Create Compatible Bitmap for Log Window!");
    //------------------------------------------------
    m_hOld_BM = (HBITMAP)SelectObject(m_hDC_LogWnd_BackBuffer, m_hBB_BM);
    //------------------------------------------------
    m_hPrevFont = (HFONT)SelectObject(m_hDC_LogWnd_BackBuffer, m_hLogWndFont);
    //------------------------------------------------
    SetTextColor(m_hDC_LogWnd_BackBuffer, RGB(255, 255, 255));
    SetBkColor(m_hDC_LogWnd_BackBuffer, RGB(1, 1, 1));
    //------------------------------------------------
    m_hBackGroundBrush = GetStockBrush(BLACK_BRUSH);
}

void CTextConsole::Initialize()
{
    inherited::Initialize();

    m_dwLastUpdateTime = Device.dwTimeGlobal;
    m_last_time = Device.dwTimeGlobal;

    m_server_info.ResetData();
}

void CTextConsole::OnDeviceInitialize()
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(Device.m_sdlWnd, &info))
    {
        switch (info.subsystem)
        {
        case SDL_SYSWM_WINDOWS:
            m_pMainWnd = &info.info.win.window;
            break;
        default: break;
        }
    }
    else
        Log("Couldn't get window information: ", SDL_GetError());

    CreateConsoleWnd();
    CreateLogWnd();

    ShowWindow(m_hConsoleWnd, SW_SHOW);
    UpdateWindow(m_hConsoleWnd);
}

void CTextConsole::Destroy()
{
    inherited::Destroy();

    SelectObject(m_hDC_LogWnd_BackBuffer, m_hPrevFont);
    SelectObject(m_hDC_LogWnd_BackBuffer, m_hOld_BM);

    if (m_hBB_BM)
        DeleteObject(m_hBB_BM);
    if (m_hOld_BM)
        DeleteObject(m_hOld_BM);
    if (m_hLogWndFont)
        DeleteObject(m_hLogWndFont);
    if (m_hPrevFont)
        DeleteObject(m_hPrevFont);
    if (m_hBackGroundBrush)
        DeleteObject(m_hBackGroundBrush);

    ReleaseDC(m_hLogWnd, m_hDC_LogWnd_BackBuffer);
    ReleaseDC(m_hLogWnd, m_hDC_LogWnd);

    DestroyWindow(m_hLogWnd);
    DestroyWindow(m_hConsoleWnd);
}

void CTextConsole::OnRender() {} // disable СConsole::OnRender()
void CTextConsole::OnPaint()
{
    RECT wRC;
    PAINTSTRUCT ps;
    BeginPaint(m_hLogWnd, &ps);

    if (/*m_bNeedUpdate*/ Device.dwFrame % 2)
    {
        // m_dwLastUpdateTime = Device.dwTimeGlobal;
        // m_bNeedUpdate = false;

        GetClientRect(m_hLogWnd, &wRC);
        DrawLog(m_hDC_LogWnd_BackBuffer, &wRC);
    }
    else
    {
        wRC = ps.rcPaint;
    }

    BitBlt(m_hDC_LogWnd, wRC.left, wRC.top, wRC.right - wRC.left, wRC.bottom - wRC.top, m_hDC_LogWnd_BackBuffer,
        wRC.left, wRC.top,
        SRCCOPY); //(FullUpdate) ? SRCCOPY : NOTSRCCOPY);
    /*
     Msg ("URect - %d:%d - %d:%d", ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
     */
    EndPaint(m_hLogWnd, &ps);
}

void CTextConsole::DrawLog(HDC hDC, RECT* pRect)
{
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);

    RECT wRC = *pRect;
    GetClientRect(m_hLogWnd, &wRC);
    FillRect(hDC, &wRC, m_hBackGroundBrush);

    int Width = wRC.right - wRC.left;
    int Height = wRC.bottom - wRC.top;
    wRC = *pRect;
    int y_top_max = (int)(0.32f * Height);

    //---------------------------------------------------------------------------------
    LPCSTR s_edt = ec().str_edit();
    LPCSTR s_cur = ec().str_before_cursor();

    u32 cur_len = xr_strlen(s_cur) + xr_strlen(ch_cursor) + 1;
    PSTR buf = (PSTR)_alloca(cur_len * sizeof(char));
    xr_strcpy(buf, cur_len, s_cur);
    xr_strcat(buf, cur_len, ch_cursor);
    buf[cur_len - 1] = 0;

    u32 cur0_len = xr_strlen(s_cur);

    int xb = 25;

    SetTextColor(hDC, RGB(255, 255, 255));
    TextOut(hDC, xb, Height - tm.tmHeight - 1, buf, cur_len - 1);
    buf[cur0_len] = 0;

    SetTextColor(hDC, RGB(0, 0, 0));
    TextOut(hDC, xb, Height - tm.tmHeight - 1, buf, cur0_len);

    SetTextColor(hDC, RGB(255, 255, 255));
    TextOut(hDC, 0, Height - tm.tmHeight - 3, ioc_prompt, xr_strlen(ioc_prompt)); // ">>> "

    SetTextColor(hDC, (COLORREF)bgr2rgb(get_mark_color(mark11)));
    TextOut(hDC, xb, Height - tm.tmHeight - 3, s_edt, xr_strlen(s_edt));

    SetTextColor(hDC, RGB(205, 205, 225));
    u32 log_line = LogFile.size() - 1;
    string16 q, q2;
    xr_itoa(log_line, q, 10);
    xr_strcpy(q2, sizeof(q2), "[");
    xr_strcat(q2, sizeof(q2), q);
    xr_strcat(q2, sizeof(q2), "]");
    u32 qn = xr_strlen(q2);

    TextOut(hDC, Width - 8 * qn, Height - tm.tmHeight - tm.tmHeight, q2, qn);

    int ypos = Height - tm.tmHeight - tm.tmHeight;
    for (int i = LogFile.size() - 1 - scroll_delta; i >= 0; --i)
    {
        ypos -= tm.tmHeight;
        if (ypos < y_top_max)
        {
            break;
        }
        LPCSTR ls = LogFile[i].c_str();

        if (!ls)
        {
            continue;
        }
        Console_mark cm = (Console_mark)ls[0];
        COLORREF c2 = (COLORREF)bgr2rgb(get_mark_color(cm));
        SetTextColor(hDC, c2);
        u8 b = (is_mark(cm)) ? 2 : 0;
        LPCSTR pOut = ls + b;

        BOOL res = TextOut(hDC, 10, ypos, pOut, xr_strlen(pOut));
        if (!res)
        {
            R_ASSERT2(0, "TextOut(..) return NULL");
        }
    }

    if (g_pGameLevel && (Device.dwTimeGlobal - m_last_time > 500))
    {
        m_last_time = Device.dwTimeGlobal;

        m_server_info.ResetData();
        g_pGameLevel->GetLevelInfo(&m_server_info);
    }

    ypos = 5;
    for (u32 i = 0; i < m_server_info.Size(); ++i)
    {
        SetTextColor(hDC, m_server_info[i].color);
        TextOut(hDC, 10, ypos, m_server_info[i].name, xr_strlen(m_server_info[i].name));

        ypos += tm.tmHeight;
        if (ypos > y_top_max)
        {
            break;
        }
    }
}
/*
void CTextConsole::IR_OnKeyboardPress( int dik ) !!!!!!!!!!!!!!!!!!!!!
{
m_bNeedUpdate = true;
inherited::IR_OnKeyboardPress( dik );
}
*/
void CTextConsole::OnFrame()
{
    inherited::OnFrame();
    /* if ( !m_bNeedUpdate && m_dwLastUpdateTime + 1000/g_svTextConsoleUpdateRate > Device.dwTimeGlobal )
     {
     return;
     }
     */ InvalidateRect(m_hConsoleWnd, NULL, FALSE);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    // m_bNeedUpdate = true;
}
