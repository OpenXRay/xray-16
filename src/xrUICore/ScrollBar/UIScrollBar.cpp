#include "pch.hpp"
#include "UIScrollBar.h"
#include "Buttons/UI3tButton.h"
#include "UIScrollBox.h"
#include "XML/UIXmlInitBase.h"
#include "XML/UITextureMaster.h"
#include "Cursor/UICursor.h"
#include "xrEngine/xr_input_xinput.h"

CUIScrollBar::CUIScrollBar()
{
    m_iMinPos = 1;
    m_iMaxPos = 1;
    m_iPageSize = 1;
    m_iStepSize = 1;
    m_iScrollPos = 0;
    m_hold_delay = 50.0f;
    m_b_enabled = true;
    m_mouse_state = 0;

    m_DecButton = new CUI3tButton();
    m_DecButton->SetAutoDelete(true);
    AttachChild(m_DecButton);
    m_IncButton = new CUI3tButton();
    m_IncButton->SetAutoDelete(true);
    AttachChild(m_IncButton);
    m_ScrollBox = new CUIScrollBox();
    m_ScrollBox->SetAutoDelete(true);
    AttachChild(m_ScrollBox);
    m_FrameBackground = new CUIFrameLineWnd();
    m_FrameBackground->SetAutoDelete(true);
    AttachChild(m_FrameBackground);
}

void CUIScrollBar::InitScrollBar(Fvector2 pos, float length, bool bIsHorizontal, LPCSTR profile)
{
    string256 _path;
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "scroll_bar.xml");

    float height = xml_doc.ReadAttribFlt(profile, 0, (bIsHorizontal) ? "height" : "height_v");
    R_ASSERT(height > 0);
    m_hold_delay = xml_doc.ReadAttribFlt(profile, 0, "hold_delay", 50.0f);

    inherited::SetWndPos(pos);
    m_bIsHorizontal = bIsHorizontal;
    m_FrameBackground->SetHorizontal(m_bIsHorizontal);
    if (m_bIsHorizontal)
    {
        inherited::SetWndSize(Fvector2().set(length, height));

        strconcat(sizeof(_path), _path, profile, ":left_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_DecButton);
        m_DecButton->SetWndPos(Fvector2().set(0, 0));

        strconcat(sizeof(_path), _path, profile, ":right_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_IncButton);
        m_IncButton->SetWndPos(Fvector2().set(length - m_IncButton->GetWidth(), 0.0f));

        m_ScrollBox->SetHorizontal(true);

        strconcat(sizeof(_path), _path, profile, ":box");
        CUIXmlInitBase::InitFrameLine(xml_doc, _path, 0, m_ScrollBox);

        strconcat(sizeof(_path), _path, profile, ":back:texture");
        LPCSTR texture = xml_doc.Read(_path, 0, "");
        R_ASSERT(texture);
        m_FrameBackground->InitTexture(texture);
        m_ScrollWorkArea = _max(0, iFloor(GetWidth() - 2 * height));
    }
    else
    {
        inherited::SetWndSize(Fvector2().set(height, length));

        strconcat(sizeof(_path), _path, profile, ":up_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_DecButton);
        m_DecButton->SetWndPos(Fvector2().set(0, 0));

        strconcat(sizeof(_path), _path, profile, ":down_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_IncButton);
        m_IncButton->SetWndPos(Fvector2().set(0.0f, length - m_IncButton->GetHeight()));

        m_ScrollBox->SetHorizontal(false);

        strconcat(sizeof(_path), _path, profile, ":box_v");
        CUIXmlInitBase::InitFrameLine(xml_doc, _path, 0, m_ScrollBox);

        strconcat(sizeof(_path), _path, profile, ":back_v:texture");
        LPCSTR texture = xml_doc.Read(_path, 0, "");
        R_ASSERT(texture);

        m_FrameBackground->InitTexture(texture);
        m_ScrollWorkArea = _max(0, iFloor(GetHeight() - 2 * height));
    }

    UpdateScrollBar();
}

//корректировка размеров скроллера
void CUIScrollBar::SetWidth(float width)
{
    if (width <= 0.0f)
        width = 1.0f;
    inherited::SetWidth(width);
    if (m_bIsHorizontal)
    {
        float work_area = float(width) - m_DecButton->GetWidth() - m_IncButton->GetWidth();
        m_ScrollWorkArea = work_area < 0.f ? 0 : int(work_area);
    }
    UpdateScrollBar();
}

void CUIScrollBar::SetHeight(float height)
{
    if (height <= 0.0f)
        height = 1.0f;
    inherited::SetHeight(height);
    if (!m_bIsHorizontal)
    {
        float work_area = float(height) - m_DecButton->GetHeight() - m_IncButton->GetHeight();
        m_ScrollWorkArea = work_area < 0.f ? 0 : int(work_area);
    }
    UpdateScrollBar();
}

void CUIScrollBar::SetStepSize(int step)
{
    m_iStepSize = step;
    UpdateScrollBar();
}

void CUIScrollBar::SetRange(int iMin, int iMax)
{
    m_iMinPos = iMin;
    m_iMaxPos = iMax;
    VERIFY(iMax >= iMin);
    if (iMax < iMin)
        iMax = iMin;
    UpdateScrollBar();
}
void CUIScrollBar::Show(bool b)
{
    if (!m_b_enabled)
        return;
    inherited::Show(b);
}

void CUIScrollBar::Enable(bool b)
{
    if (!m_b_enabled)
        return;
    inherited::Enable(b);
}

void CUIScrollBar::UpdateScrollBar()
{
    if (IsShown())
    {
        //уcтановить размер и положение каретки
        if (m_iMaxPos == m_iMinPos)
            m_iMaxPos++;
        float box_sz = float(m_ScrollWorkArea) * float(m_iPageSize ? m_iPageSize : 1) / float(m_iMaxPos - m_iMinPos);
        if (IsRelevant())
        {
            if (m_bIsHorizontal)
            {
                // set width
                clamp(box_sz, _min(GetHeight(), GetWidth() - m_IncButton->GetWidth() - m_DecButton->GetWidth()),
                    GetWidth() - m_IncButton->GetWidth() - m_DecButton->GetWidth());
                m_ScrollBox->SetWidth(box_sz);
                m_ScrollBox->SetHeight(GetHeight());
                // set pos
                int pos = PosViewFromScroll(iFloor(m_ScrollBox->GetWidth()), iFloor(GetHeight()));
                m_ScrollBox->SetWndPos(Fvector2().set(float(pos), m_ScrollBox->GetWndRect().top));
                m_IncButton->SetWndPos(Fvector2().set(GetWidth() - m_IncButton->GetWidth(), 0.0f));
            }
            else
            {
                // set height
                clamp(box_sz, _min(GetWidth(), GetHeight() - m_IncButton->GetHeight() - m_DecButton->GetHeight()),
                    GetHeight() - m_IncButton->GetHeight() - m_DecButton->GetHeight());
                m_ScrollBox->SetHeight(box_sz);
                m_ScrollBox->SetWidth(GetWidth());
                // set pos
                int pos = PosViewFromScroll(iFloor(m_ScrollBox->GetHeight()), iFloor(GetWidth()));
                m_ScrollBox->SetWndPos(Fvector2().set(m_ScrollBox->GetWndRect().left, float(pos)));
                m_IncButton->SetWndPos(Fvector2().set(0.0f, GetHeight() - m_IncButton->GetHeight()));
            }
        }
    }
    if (IsRelevant())
    {
        m_ScrollBox->SetTextureColor(color_rgba(255, 255, 255, 255));
    }
    else
    {
        m_ScrollBox->SetTextureColor(color_rgba(255, 255, 255, 0));
    }

    ClampByViewRect();
}

u32 last_hold_time = 0;

bool CUIScrollBar::OnKeyboardHold(int dik)
{
    if (dik == MOUSE_1 && (last_hold_time + m_hold_delay) < Device.dwTimeContinual) // 100
    {
        if (OnMouseDownEx())
        {
            last_hold_time = Device.dwTimeContinual;
            return true;
        }
    }
    return inherited::OnKeyboardHold(dik);
}

bool CUIScrollBar::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    switch (mouse_action)
    {
    case WINDOW_MOUSE_WHEEL_DOWN:
        TryScrollInc(true);
        return true;
        break;
    case WINDOW_MOUSE_WHEEL_UP:
        TryScrollDec(true);
        return true;
        break;
    case WINDOW_LBUTTON_UP: m_mouse_state = 0; break;
    };

    return inherited::OnMouseAction(x, y, mouse_action);
}

bool CUIScrollBar::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1)
    {
        if (OnMouseDownEx())
        {
            return true;
        }
    }
    return inherited::OnMouseDown(mouse_btn);
}
bool CUIScrollBar::OnMouseDownEx()
{
    Fvector2 cursor_pos = GetUICursor().GetCursorPosition();
    Frect box_rect, dec_rect, inc_rect;
    m_ScrollBox->GetAbsoluteRect(box_rect);
    m_DecButton->GetAbsoluteRect(dec_rect);
    m_IncButton->GetAbsoluteRect(inc_rect);

    if (dec_rect.in(cursor_pos) && (m_mouse_state != 2))
    {
        TryScrollDec();
        m_mouse_state = 1;
        return true;
    }

    if (inc_rect.in(cursor_pos) && (m_mouse_state != 1))
    {
        TryScrollInc();
        m_mouse_state = 2;
        return true;
    }

    Frect dec2_rect, inc2_rect;
    if (m_bIsHorizontal)
    {
        dec2_rect.set(dec_rect.x2, dec_rect.y1, box_rect.x1, box_rect.y2);
        inc2_rect.set(box_rect.x2, box_rect.y1, inc_rect.x1, inc_rect.y2);
    }
    else
    {
        dec2_rect.set(dec_rect.x1, dec_rect.y2, box_rect.x2, box_rect.y1);
        inc2_rect.set(box_rect.x1, box_rect.y2, inc_rect.x2, inc_rect.y1);
    }

    if (dec2_rect.in(cursor_pos) && (m_mouse_state != 2))
    {
        TryScrollDec(true);
        m_mouse_state = 1;
        return true;
    }

    if (inc2_rect.in(cursor_pos) && (m_mouse_state != 1))
    {
        TryScrollInc(true);
        m_mouse_state = 2;
        return true;
    }
    return false;
}

void CUIScrollBar::OnMouseUp(int mouse_btn) { m_mouse_state = 0; }
void CUIScrollBar::ClampByViewRect()
{
    if (m_bIsHorizontal)
    {
        if (m_ScrollBox->GetWndRect().left <= m_DecButton->GetWidth())
            m_ScrollBox->SetWndPos(Fvector2().set(m_DecButton->GetWidth(), m_ScrollBox->GetWndRect().top));
        else if (m_ScrollBox->GetWndRect().right >= m_IncButton->GetWndPos().x)
            m_ScrollBox->SetWndPos(Fvector2().set(
                m_IncButton->GetWndRect().left - m_ScrollBox->GetWidth(), m_ScrollBox->GetWndRect().top));
    }
    else
    {
        // limit vertical position (TOP) by position of button
        if (m_ScrollBox->GetWndRect().top <= m_DecButton->GetHeight())
            m_ScrollBox->SetWndPos(Fvector2().set(m_ScrollBox->GetWndRect().left, m_DecButton->GetHeight()));
        // limit vertical position (BOTTOM) by position of button
        else if (m_ScrollBox->GetWndRect().bottom >= m_IncButton->GetWndPos().y)
            m_ScrollBox->SetWndPos(
                Fvector2().set(m_ScrollBox->GetWndRect().left, m_IncButton->GetWndPos().y - m_ScrollBox->GetHeight()));
    }
}

void CUIScrollBar::SetPosScrollFromView(float view_pos, float view_size, float view_offs)
{
    int scroll_size = ScrollSize();
    float pos = view_pos - view_offs;
    float work_size = m_ScrollWorkArea - view_size;
    SetScrollPosClamped(work_size ? iFloor(((pos / work_size) * (scroll_size) + m_iMinPos)) : 0);
}

int CUIScrollBar::PosViewFromScroll(int view_size, int view_offs)
{
    int work_size = m_ScrollWorkArea - view_size;
    int scroll_size = ScrollSize();
    return scroll_size ? (m_iScrollPos * work_size + scroll_size * view_offs - m_iMinPos * work_size) / scroll_size : 0;
}

void CUIScrollBar::SetScrollPosClamped(int iPos)
{
    m_iScrollPos = iPos;
    clamp(m_iScrollPos, m_iMinPos, m_iMaxPos - m_iPageSize + 1);
}

void CUIScrollBar::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_DecButton)
    {
        if (msg == BUTTON_CLICKED || msg == BUTTON_DOWN)
        {
            TryScrollDec();
        }
    }
    else if (pWnd == m_IncButton)
    {
        if (msg == BUTTON_CLICKED || msg == BUTTON_DOWN)
        {
            TryScrollInc();
        }
    }
    else if (pWnd == m_ScrollBox)
    {
        if (msg == SCROLLBOX_MOVE)
        {
            //вычислить новое положение прокрутки
            ClampByViewRect();
            if (m_bIsHorizontal)
            {
                SetPosScrollFromView(m_ScrollBox->GetWndPos().x, m_ScrollBox->GetWidth(), GetHeight());
                if (GetMessageTarget())
                    GetMessageTarget()->SendMessage(this, SCROLLBAR_HSCROLL);
            }
            else
            {
                SetPosScrollFromView(m_ScrollBox->GetWndPos().y, m_ScrollBox->GetHeight(), GetWidth());
                if (GetMessageTarget())
                    GetMessageTarget()->SendMessage(this, SCROLLBAR_VSCROLL);
            }
        }
    }
    CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUIScrollBar::TryScrollInc(bool by_scrollbox)
{
    if (ScrollInc(by_scrollbox))
        if (m_bIsHorizontal)
            GetMessageTarget()->SendMessage(this, SCROLLBAR_HSCROLL);
        else
            GetMessageTarget()->SendMessage(this, SCROLLBAR_VSCROLL);
}

void CUIScrollBar::TryScrollDec(bool by_scrollbox)
{
    if (ScrollDec(by_scrollbox))
        if (m_bIsHorizontal)
            GetMessageTarget()->SendMessage(this, SCROLLBAR_HSCROLL);
        else
            GetMessageTarget()->SendMessage(this, SCROLLBAR_VSCROLL);
}

bool CUIScrollBar::ScrollDec(bool by_scrollbox)
{
    if (m_iScrollPos > m_iMinPos)
    {
        if (m_iScrollPos > m_iStepSize)
        {
            if (by_scrollbox)
                SetScrollPos(m_iScrollPos - m_iStepSize * 4);
            else
                SetScrollPos(m_iScrollPos - m_iStepSize);
        }
        else
            SetScrollPos(0);

        return true;
    }

    return false;
}

bool CUIScrollBar::ScrollInc(bool by_scrollbox)
{
    if (m_iScrollPos <= (m_iMaxPos - m_iPageSize + 1))
    {
        {
            if (by_scrollbox)
                SetScrollPos(m_iScrollPos + m_iStepSize * 4);
            else
                SetScrollPos(m_iScrollPos + m_iStepSize);
        }
        return true;
    }

    return false;
}

void CUIScrollBar::Reset()
{
    ResetAll();
    inherited::Reset();
}

bool CUIScrollBar::IsRelevant()
{
    bool b_can_inc = (m_iScrollPos <= (m_iMaxPos - m_iPageSize));
    bool b_can_dec = (m_iScrollPos > m_iMinPos);
    return b_can_inc || b_can_dec;
}

void CUIScrollBar::Draw()
{
    if (m_bIsHorizontal)
    {
        float size = GetWidth() - m_DecButton->GetWidth() - m_IncButton->GetWidth();

        m_FrameBackground->SetWndSize(Fvector2().set(size, GetHeight()));
        m_FrameBackground->SetWndPos(Fvector2().set(m_DecButton->GetWidth(), 0.0f));
    }
    else
    {
        float size = GetHeight() - m_IncButton->GetHeight() - m_DecButton->GetHeight();

        m_FrameBackground->SetWndSize(Fvector2().set(GetWidth(), size));
        m_FrameBackground->SetWndPos(Fvector2().set(0.0f, m_DecButton->GetHeight()));
    }
    inherited::Draw();
}

void CUIScrollBar::Refresh() { SendMessage(m_ScrollBox, SCROLLBOX_MOVE, NULL); }
