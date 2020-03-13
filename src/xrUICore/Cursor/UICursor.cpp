#include "pch.hpp"
#include "UICursor.h"

#include "Static/UIStatic.h"
#include "Buttons/UIBtnHint.h"
#include "xrEngine/IInputReceiver.h"
#include "xrEngine/xr_input.h"
#include "SDL_syswm.h"

#define C_DEFAULT color_xrgb(0xff, 0xff, 0xff)

CUICursor::CUICursor() : m_static(NULL), m_bound_to_system_cursor(false)
{
    bVisible = false;
    vPrevPos.set(0.0f, 0.0f);
    vPos.set(0.f, 0.f);
    InitInternal();
    Device.seqRender.Add(this, -3 /*2*/);
}
//--------------------------------------------------------------------
CUICursor::~CUICursor()
{
    xr_delete(m_static);
    Device.seqRender.Remove(this);
}

void CUICursor::OnDeviceReset()
{
    correction.x = UI_BASE_WIDTH / (float)Device.m_rcWindowClient.w;
    correction.y = UI_BASE_HEIGHT / (float)Device.m_rcWindowClient.h;

    SDL_Rect display;
    if (0 == SDL_GetDisplayBounds(0, &display))
    {
        const u32 screen_size_x = display.w - display.x;
        const u32 screen_size_y = display.h - display.y;
        m_bound_to_system_cursor = screen_size_y >= Device.dwHeight && screen_size_x >= Device.dwWidth;
    }
    if (m_bound_to_system_cursor) // sanity
        Device.UpdateWindowRects();
}

void CUICursor::OnUIReset()
{
    xr_delete(m_static);
    InitInternal();
}

void CUICursor::Show()
{
    bVisible = true;
}

void CUICursor::Hide()
{
    bVisible = false;
}

void CUICursor::InitInternal()
{
    m_static = new CUIStatic();
    m_static->InitTextureEx("ui" DELIMITER "ui_ani_cursor", "hud" DELIMITER "cursor");
    Frect rect;
    rect.set(0.0f, 0.0f, 40.0f, 40.0f);
    m_static->SetTextureRect(rect);
    Fvector2 sz;
    sz.set(rect.rb);
    sz.x *= UI().get_current_kx();

    m_static->SetWndSize(sz);
    m_static->SetStretchTexture(true);

    OnDeviceReset();
}

//--------------------------------------------------------------------
u32 last_render_frame = 0;
void CUICursor::OnRender()
{
    g_btnHint->OnRender();
    g_statHint->OnRender();

    if (!IsVisible())
        return;
#ifdef DEBUG
    VERIFY(last_render_frame != Device.dwFrame);
    last_render_frame = Device.dwFrame;

    if (bDebug)
    {
        CGameFont* F = UI().Font().pFontDI;
        F->SetAligment(CGameFont::alCenter);
        F->SetHeightI(0.02f);
        F->OutSetI(0.f, -0.9f);
        F->SetColor(0xffffffff);
        Fvector2 pt = GetCursorPosition();
        F->OutNext("%f-%f", pt.x, pt.y);
    }
#endif

    m_static->SetWndPos(vPos);
    m_static->Update();
    m_static->Draw();
}

Fvector2 CUICursor::GetCursorPosition() { return vPos; }
Fvector2 CUICursor::GetCursorPositionDelta()
{
    Fvector2 res_delta;

    res_delta.x = vPos.x - vPrevPos.x;
    res_delta.y = vPos.y - vPrevPos.y;
    return res_delta;
}

void CUICursor::UpdateCursorPosition(int _dx, int _dy)
{
    vPrevPos = vPos;
    if (m_bound_to_system_cursor)
    {
        Ivector2 pti;
        IInputReceiver::IR_GetMousePosWindow(pti);
        vPos.x = (float)pti.x * correction.x;
        vPos.y = (float)pti.y * correction.y;
    }
    else
    {
        float sens = 1.0f;
        vPos.x += _dx * sens * correction.x;
        vPos.y += _dy * sens * correction.y;
    }
    clamp(vPos.x, 0.f, UI_BASE_WIDTH);
    clamp(vPos.y, 0.f, UI_BASE_HEIGHT);
}

void CUICursor::SetUICursorPosition(Fvector2 pos)
{
    vPos = pos;
    Ivector2 p;
    p.x = iFloor(vPos.x / correction.x);
    p.y = iFloor(vPos.y / correction.y);
    SDL_WarpMouseInWindow(Device.m_sdlWnd, p.x, p.y);
}
