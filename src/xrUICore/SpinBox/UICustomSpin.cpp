// file:		UIustomSpin.cpp
// description:	base class for CSpinNum & CSpinText
// created:		15.06.2005
// author:		Serge Vynnychenko
// mail:		narrator@gsc-game.kiev.ua
//
// copyright 2005 GSC Game World

#include "pch.hpp"
#include "Buttons/UI3tButton.h"
#include "Windows/UIFrameLineWnd.h"
#include "Lines/UILines.h"
#include "UICustomSpin.h"

#define SPIN_HEIGHT 20.0f
#define BTN_SIZE_X 11.0f
#define BTN_SIZE_Y 8.0f

CUICustomSpin::CUICustomSpin()
{
    m_pFrameLine = new CUIFrameLineWnd();
    m_pBtnUp = new CUI3tButton();
    m_pBtnDown = new CUI3tButton();
    m_pLines = new CUILines();

    m_pFrameLine->SetAutoDelete(true);
    m_pBtnUp->SetAutoDelete(true);
    m_pBtnDown->SetAutoDelete(true);

    AttachChild(m_pFrameLine);
    AttachChild(m_pBtnUp);
    AttachChild(m_pBtnDown);
    m_pLines->SetTextAlignment(CGameFont::alLeft);
    m_pLines->SetVTextAlignment(valCenter);
    m_pLines->SetFont(UI().Font().pFontLetterica16Russian);
    m_pLines->SetTextColor(color_argb(255, 235, 219, 185));

    m_time_begin = 0;
    m_p_delay = 500;
    m_u_delay = 0;

    m_textColor[0] = color_argb(255, 235, 219, 185);
    m_textColor[1] = color_argb(255, 100, 100, 100);
}

CUICustomSpin::~CUICustomSpin() { xr_delete(m_pLines); }
void CUICustomSpin::InitSpin(Fvector2 pos, Fvector2 size)
{
    CUIWindow::SetWndPos(pos);
    CUIWindow::SetWndSize(Fvector2().set(size.x, SPIN_HEIGHT));

    m_pFrameLine->SetWndPos(Fvector2().set(0, 0));
    m_pFrameLine->SetWndSize(Fvector2().set(size.x, SPIN_HEIGHT));
    m_pFrameLine->InitTexture("ui_inGame2_spin_box", "hud" DELIMITER "default");

    m_pBtnUp->InitButton(Fvector2().set(size.x - BTN_SIZE_X - 2.0f, 1.0f), Fvector2().set(BTN_SIZE_X, BTN_SIZE_Y));
    m_pBtnUp->InitTexture("ui_inGame2_spin_box_button_top");

    m_pBtnDown->InitButton(
        Fvector2().set(size.x - BTN_SIZE_X - 2.0f, BTN_SIZE_Y + 2.0f), Fvector2().set(BTN_SIZE_X, BTN_SIZE_Y));
    m_pBtnDown->InitTexture("ui_inGame2_spin_box_button_bottom");

    m_pLines->m_wndPos.set(Fvector2().set(0, 0));
    m_pLines->m_wndSize.set(Fvector2().set(size.x - BTN_SIZE_X - 10.0f, SPIN_HEIGHT));
}

void CUICustomSpin::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (BUTTON_CLICKED == msg)
    {
        if (m_pBtnUp == pWnd)
        {
            OnBtnUpClick();
        }
        else if (m_pBtnDown == pWnd)
        {
            OnBtnDownClick();
        }
    }
}

void CUICustomSpin::Enable(bool status)
{
    CUIWindow::Enable(status);
    m_pBtnDown->Enable(status);
    m_pBtnUp->Enable(status);

    if (!status)
        m_pLines->SetTextColor(m_textColor[0]); // enabled color
    else
        m_pLines->SetTextColor(m_textColor[1]); // disabled color
}

void CUICustomSpin::OnBtnUpClick() { GetMessageTarget()->SendMessage(this, BUTTON_CLICKED); }
void CUICustomSpin::OnBtnDownClick() { GetMessageTarget()->SendMessage(this, BUTTON_CLICKED); }
void CUICustomSpin::Draw()
{
    CUIWindow::Draw();
    Fvector2 pos;
    GetAbsolutePos(pos);
    m_pLines->Draw(pos.x + 3, pos.y);
}

void CUICustomSpin::Update()
{
    CUIWindow::Update();
    if (!m_pBtnUp->CursorOverWindow())
        m_pBtnUp->SetButtonState(CUIButton::BUTTON_NORMAL);
    if (!m_pBtnDown->CursorOverWindow())
        m_pBtnDown->SetButtonState(CUIButton::BUTTON_NORMAL);

    if (CUIButton::BUTTON_PUSHED == m_pBtnUp->GetButtonState() && m_pBtnUp->CursorOverWindow())
    {
        if (m_time_begin < Device.dwTimeContinual - m_p_delay)
        {
            m_time_begin = Device.dwTimeContinual;
            float tmp = float(m_u_delay);
            float step = powf(tmp, 0.7f);
            while (tmp > 0)
            {
                IncVal();
                tmp -= step;
            };

            m_u_delay += 50;

            if (m_p_delay > 50)
                m_p_delay -= 50;
        }
    }
    else if (CUIButton::BUTTON_PUSHED == m_pBtnDown->GetButtonState() && m_pBtnDown->CursorOverWindow())
    {
        if (m_time_begin < Device.dwTimeContinual - m_p_delay)
        {
            m_time_begin = Device.dwTimeContinual;
            float tmp = float(m_u_delay);
            float step = powf(tmp, 0.7f);
            while (tmp > 0)
            {
                DecVal();
                tmp -= step;
            };

            m_u_delay += 50;

            if (m_p_delay > 50)
                m_p_delay -= 50;
        }
    }
    else
    {
        m_p_delay = 500;
        m_u_delay = 0;
        m_time_begin = 0;
    }

    if (IsEnabled())
    {
        m_pBtnUp->Enable(CanPressUp());
        m_pBtnDown->Enable(CanPressDown());
        m_pLines->SetTextColor(m_textColor[0]);
    }
    else
    {
        m_pBtnUp->Enable(false);
        m_pBtnDown->Enable(false);
        m_pLines->SetTextColor(m_textColor[1]);
    }
}

LPCSTR CUICustomSpin::GetText() { return m_pLines->GetText(); }
void CUICustomSpin::SetTextColor(u32 color) { m_textColor[0] = color; }
void CUICustomSpin::SetTextColorD(u32 color) { m_textColor[1] = color; }
