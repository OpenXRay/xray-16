#include "pch.hpp"
#include "UIFixedScrollBar.h"
#include "Buttons/UI3tButton.h"
#include "Windows/UIFrameLineWnd.h"
#include "UIScrollBox.h"
#include "XML/UIXmlInitBase.h"
#include "Cursor/UICursor.h"
#include "xrEngine/xr_input_xinput.h"

CUIFixedScrollBar::CUIFixedScrollBar()
{
    m_ScrollBox = new CUI3tButton();
    m_ScrollBox->SetAutoDelete(true);
    AttachChild(m_ScrollBox);
}

CUIFixedScrollBar::~CUIFixedScrollBar(void) {}
void CUIFixedScrollBar::InitScrollBar(Fvector2 pos, bool horizontal, LPCSTR profile)
{
    string256 _path;
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "scroll_bar.xml");

    float width = xml_doc.ReadAttribFlt(profile, 0, "width", 17.0f);
    float height = xml_doc.ReadAttribFlt(profile, 0, "height", 17.0f);
    float width_v = xml_doc.ReadAttribFlt(profile, 0, "width_v", 17.0f);
    float height_v = xml_doc.ReadAttribFlt(profile, 0, "height_v", 17.0f);
    m_hold_delay = xml_doc.ReadAttribFlt(profile, 0, "hold_delay", 50.0f);
    m_ScrollBoxOffset.x = xml_doc.ReadAttribInt(profile, 0, "scroll_box_offset_x", 0);
    m_ScrollBoxOffset.y = xml_doc.ReadAttribInt(profile, 0, "scroll_box_offset_y", 0);

    inherited::SetWndPos(pos);
    m_bIsHorizontal = horizontal;
    if (m_bIsHorizontal)
    {
        inherited::SetWndSize(Fvector2().set(width, height));

        strconcat(sizeof(_path), _path, profile, ":left_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_DecButton);

        strconcat(sizeof(_path), _path, profile, ":right_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_IncButton);

        strconcat(sizeof(_path), _path, profile, ":box");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_ScrollBox);

        strconcat(sizeof(_path), _path, profile, ":back");
        CUIXmlInitBase::InitFrameLine(xml_doc, _path, 0, m_FrameBackground);

        m_ScrollWorkArea = _max(0, iFloor(GetWidth() - 2 * height));
    }
    else
    {
        inherited::SetWndSize(Fvector2().set(width_v, height_v));

        strconcat(sizeof(_path), _path, profile, ":up_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_DecButton);

        strconcat(sizeof(_path), _path, profile, ":down_arrow");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_IncButton);

        strconcat(sizeof(_path), _path, profile, ":box_v");
        CUIXmlInitBase::Init3tButton(xml_doc, _path, 0, m_ScrollBox);

        strconcat(sizeof(_path), _path, profile, ":back_v");
        CUIXmlInitBase::InitFrameLine(xml_doc, _path, 0, m_FrameBackground);

        m_ScrollWorkArea = _max(0, iFloor(GetHeight() - 2 * width_v));
    }
    UpdateScrollBar();
}
void CUIFixedScrollBar::UpdateScrollBar()
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
                    GetWidth() - m_IncButton->GetWidth() - m_DecButton->GetWidth() - 2 * m_ScrollBoxOffset.x);
                m_ScrollBox->SetWidth(box_sz);
                // set pos
                int pos = PosViewFromScroll(iFloor(box_sz), iFloor(GetHeight()));
                m_ScrollBox->SetWndPos(Fvector2().set(float(pos), m_ScrollBox->GetWndRect().top));
                m_IncButton->SetWndPos(Fvector2().set(GetWidth() - m_IncButton->GetWidth(), 0.0f));
            }
            else
            {
                // set height
                clamp(box_sz, _min(GetWidth(), GetHeight() - m_IncButton->GetHeight() - m_DecButton->GetHeight()),
                    GetHeight() - m_IncButton->GetHeight() - m_DecButton->GetHeight() - 2 * m_ScrollBoxOffset.y);
                m_ScrollBox->SetHeight(box_sz);
                // set pos
                int pos = PosViewFromScroll(iFloor(box_sz), iFloor(GetWidth()));
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
void CUIFixedScrollBar::ClampByViewRect()
{
    if (m_bIsHorizontal)
    {
        if (m_ScrollBox->GetWndRect().left <= m_DecButton->GetWidth() + m_ScrollBoxOffset.x)
            m_ScrollBox->SetWndPos(
                Fvector2().set(m_ScrollBoxOffset.x + m_DecButton->GetWidth(), m_ScrollBox->GetWndRect().top));
        else if (m_ScrollBox->GetWndRect().right >= m_IncButton->GetWndPos().x - m_ScrollBoxOffset.x)
            m_ScrollBox->SetWndPos(
                Fvector2().set(m_IncButton->GetWndRect().left - m_ScrollBox->GetWidth() - m_ScrollBoxOffset.x,
                    m_ScrollBox->GetWndRect().top));
    }
    else
    {
        if (m_ScrollBox->GetWndRect().top <= m_DecButton->GetHeight() + m_ScrollBoxOffset.y)
            m_ScrollBox->SetWndPos(
                Fvector2().set(m_ScrollBox->GetWndRect().left, m_ScrollBoxOffset.y + m_DecButton->GetHeight()));
        else if (m_ScrollBox->GetWndRect().bottom >= m_IncButton->GetWndPos().y - m_ScrollBoxOffset.y)
            m_ScrollBox->SetWndPos(Fvector2().set(m_ScrollBox->GetWndRect().left,
                m_IncButton->GetWndPos().y - m_ScrollBox->GetHeight() - m_ScrollBoxOffset.y));
    }
}

u32 last_hold_tm = 0;
bool CUIFixedScrollBar::OnKeyboardHold(int dik)
{
    if (dik == MOUSE_1 && (last_hold_tm + m_hold_delay) < Device.dwTimeContinual)
    {
        if (OnMouseDownEx())
        {
            last_hold_tm = Device.dwTimeContinual;
            return true;
        }
    }
    return inherited::OnKeyboardHold(dik);
}

bool CUIFixedScrollBar::OnMouseAction(float x, float y, EUIMessages mouse_action)
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
    case WINDOW_LBUTTON_UP:
        SetCapture(m_ScrollBox, false);
        m_mouse_state = 0;
        return true;
    case WINDOW_LBUTTON_DOWN: SetCapture(m_ScrollBox, true); return true;
    case WINDOW_MOUSE_MOVE:
    {
        bool im_capturer = (GetMouseCapturer() == m_ScrollBox);
        bool cursor_over = false;
        Fvector2 cursor_pos = GetUICursor().GetCursorPosition();
        Frect box_rect;
        m_ScrollBox->GetAbsoluteRect(box_rect);
        if (box_rect.in(cursor_pos))
            cursor_over = true;

        // bool over_x = ( x >= -512.0f && x < (m_ScrollBox->GetWidth()  + 512.0f) );
        // bool over_y = ( y >= -512.0f && y < (m_ScrollBox->GetHeight() + 512.0f) );
        // if ( over_x && over_y )
        //	cursor_over = true;

        if (im_capturer && cursor_over)
        {
            Fvector2 pos = m_ScrollBox->GetWndPos();
            Fvector2 delta = GetUICursor().GetCursorPositionDelta();
            if (m_bIsHorizontal)
                pos.x += delta.x;
            else
                pos.y += delta.y;

            m_ScrollBox->SetWndPos(pos);
            m_ScrollBox->GetMessageTarget()->SendMessage(m_ScrollBox, SCROLLBOX_MOVE);
        }
        if (!cursor_over)
        {
            SetCapture(m_ScrollBox, false);
        }
        return true;
    }
    };

    return inherited::OnMouseAction(x, y, mouse_action);
}

bool CUIFixedScrollBar::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1 && OnMouseDownEx())
        return true;

    return inherited::OnMouseDown(mouse_btn);
}
bool CUIFixedScrollBar::OnMouseDownEx()
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
        TryScrollDec(false);
        //		m_mouse_state = 1;
        return true;
    }
    if (inc2_rect.in(cursor_pos) && (m_mouse_state != 1))
    {
        TryScrollInc(false);
        //		m_mouse_state = 2;
        return true;
    }
    return false;
}
void CUIFixedScrollBar::OnMouseUp(int mouse_btn) { m_mouse_state = 0; }
void CUIFixedScrollBar::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_DecButton)
    {
        if (msg == BUTTON_CLICKED || msg == BUTTON_DOWN)
            TryScrollDec();
    }
    else if (pWnd == m_IncButton)
    {
        if (msg == BUTTON_CLICKED || msg == BUTTON_DOWN)
            TryScrollInc();
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

void CUIFixedScrollBar::SetPosScrollFromView(float view_pos, float view_size, float view_offs)
{
    int scroll_size = ScrollSize();
    float pos = view_pos - view_offs;
    float work_size = m_ScrollWorkArea - view_size;
    m_iScrollPos = work_size ? iFloor(((pos / work_size) * (scroll_size) + m_iMinPos)) : 0;
    clamp(m_iScrollPos, m_iMinPos, m_iMaxPos - m_iPageSize + 1);
    UpdateScrollBar();
}
