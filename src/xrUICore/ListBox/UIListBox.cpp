#include "pch.hpp"
#include "UIListBox.h"
#include "UIListBoxItem.h"
#include "ScrollBar/UIScrollBar.h"
#include "Static/UIStatic.h"
#include "xrEngine/StringTable/IStringTable.h"

CUIListBox::CUIListBox()
{
    m_pFont = NULL;
    m_flags.set(eItemsSelectabe, TRUE);

    m_def_item_height = 20;
    m_text_color = 0xff000000;

    m_bImmediateSelection = false;

    SetFixedScrollBar(false);
    InitScrollView();
}

void CUIListBox::SetSelectionTexture(LPCSTR texture) { m_selection_texture = texture; }
bool CUIListBox::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    if (CUIWindow::OnMouseAction(x, y, mouse_action))
        return true;

    switch (mouse_action)
    {
    case WINDOW_MOUSE_WHEEL_UP:
        m_VScrollBar->TryScrollDec();
        return true;
        break;
    case WINDOW_MOUSE_WHEEL_DOWN:
        m_VScrollBar->TryScrollInc();
        return true;
        break;
    };
    return false;
}

CUIListBoxItem* CUIListBox::AddTextItem(LPCSTR text)
{
    CUIListBoxItem* pItem = AddItem();

    pItem->SetWndSize(Fvector2().set(GetDesiredChildWidth(), m_def_item_height));
    pItem->SetTextColor(m_text_color);
    pItem->SetText(gStringTable->translate(text).c_str());
    pItem->GetTextItem()->SetWidth(GetDesiredChildWidth());
    return pItem;
}

CUIListBoxItem* CUIListBox::AddItem()
{
    CUIListBoxItem* item = new CUIListBoxItem(m_def_item_height);
    item->InitFrameLineWnd(Fvector2().set(0, 0), Fvector2().set(GetDesiredChildWidth() - 5, m_def_item_height));
    item->GetTextItem()->SetWidth(GetDesiredChildWidth());
    item->SetWidth(GetDesiredChildWidth());

    if (m_selection_texture.size())
        item->InitTexture(m_selection_texture.c_str(), "hud" DELIMITER "default");
    else
        item->InitDefault();

    item->SetFont(GetFont());
    item->SetSelected(false);
    item->SetMessageTarget(this);
    AddWindow(item, true);
    return item;
}

void CUIListBox::AddExistingItem(CUIListBoxItem* item)
{
    item->InitFrameLineWnd(Fvector2().set(0, 0), Fvector2().set(GetDesiredChildWidth() - 5, m_def_item_height));
    item->SetWidth(GetDesiredChildWidth());

    if (m_selection_texture.size())
        item->InitTexture(m_selection_texture.c_str(), "hud" DELIMITER "default");
    else
        item->InitDefault();

    item->SetSelected(false);
    item->SetMessageTarget(this);
    AddWindow(item, true);
}

void CUIListBox::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (m_pad->IsChild(pWnd))
    {
        switch (msg)
        {
        case LIST_ITEM_SELECT: GetMessageTarget()->SendMessage(this, LIST_ITEM_SELECT, pData); break;
        case LIST_ITEM_CLICKED: GetMessageTarget()->SendMessage(this, LIST_ITEM_CLICKED, pData); break;
        case LIST_ITEM_FOCUS_RECEIVED:
            if (m_bImmediateSelection)
                SetSelected(pWnd);
            break;
        }
    }

    CUIScrollView::SendMessage(pWnd, msg, pData);
}

CUIListBoxItem* CUIListBox::GetSelectedItem()
{
    CUIWindow* w = GetSelected();

    if (w)
        return smart_cast<CUIListBoxItem*>(w);
    else
        return NULL;
}

LPCSTR CUIListBox::GetSelectedText()
{
    CUIWindow* w = GetSelected();

    if (w)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(w);
        return item->GetText();
    }
    else
        return NULL;
}

u32 CUIListBox::GetSelectedIDX()
{
    u32 _idx = 0;
    CUIWindow* w = GetSelected();

    for (auto it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(*it);
        if (item)
        {
            if (*it == w)
                return _idx;

            ++_idx;
        }
    }
    return u32(-1);
}

LPCSTR CUIListBox::GetText(int idx)
{
    if (idx == -1)
        return NULL;

    CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(GetItem(idx));
    return item->GetText();
}

void CUIListBox::MoveSelectedUp()
{
    CUIWindow* w = GetSelected();
    if (!w)
        return;

    WINDOW_LIST::reverse_iterator it = m_pad->GetChildWndList().rbegin();
    WINDOW_LIST::reverse_iterator it_e = m_pad->GetChildWndList().rend();
    WINDOW_LIST::reverse_iterator it_prev = it;

    for (; it != it_e; ++it)
    {
        if (*it == w)
        {
            it_prev = it;
            ++it_prev;
            if (it_prev == it_e)
                break;

            std::swap(*it, *it_prev);
            ForceUpdate();
            break;
        }
    }
}

void CUIListBox::MoveSelectedDown()
{
    CUIWindow* w = GetSelected();
    if (!w)
        return;
    //.	R_ASSERT(!m_flags.test(CUIScrollView::eMultiSelect));
    auto it = m_pad->GetChildWndList().begin();
    auto it_e = m_pad->GetChildWndList().end();

    for (; it != it_e; ++it)
    {
        if (*it == w)
        {
            auto it_next = it;
            ++it_next;
            if (it_next == it_e)
                break;

            std::swap(*it, *it_next);
            ForceUpdate();
            break;
        }
    }
}

void CUIListBox::SetSelectedIDX(u32 idx) { SetSelected(GetItemByIDX(idx)); }
void CUIListBox::SetSelectedTAG(u32 tag_val) { SetSelected(GetItemByTAG(tag_val)); }
void CUIListBox::SetSelectedText(LPCSTR txt) { SetSelected(GetItemByText(txt)); }
int CUIListBox::GetIdxByTAG(u32 tag_val)
{
    int result = -1;

    for (auto it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(*it);
        if (item)
        {
            if (result == -1)
                result = 0;
            else
                ++result;

            if (item->GetTAG() == tag_val)
                break;
        }
    }
    return result;
}

CUIListBoxItem* CUIListBox::GetItemByTAG(u32 tag_val)
{
    for (auto it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(*it);
        if (item)
        {
            if (item->GetTAG() == tag_val)
                return item;
        }
    }
    return NULL;
}

CUIListBoxItem* CUIListBox::GetItemByIDX(int idx)
{
    int _idx = 0;
    for (auto it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(*it);
        if (item)
        {
            if (_idx == idx)
                return item;
            ++_idx;
        }
    }
    return NULL;
}

CUIListBoxItem* CUIListBox::GetItemByText(LPCSTR txt)
{
    for (auto it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(*it);
        if (item)
        {
            if (0 == xr_strcmp(item->GetText(), txt))
                return item;
        }
    }
    return NULL;
}

void CUIListBox::SetItemHeight(float h) { m_def_item_height = h; }
float CUIListBox::GetItemHeight() { return m_def_item_height; }
void CUIListBox::SetTextColor(u32 color) { m_text_color = color; }
u32 CUIListBox::GetTextColor() { return m_text_color; }
void CUIListBox::SetFont(CGameFont* pFont) { m_pFont = pFont; }
CGameFont* CUIListBox::GetFont() { return m_pFont; }
float CUIListBox::GetLongestLength()
{
    float len = 0;
    for (auto it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
    {
        CUIListBoxItem* item = smart_cast<CUIListBoxItem*>(*it);
        if (item)
        {
            float tmp_len = item->GetFont()->SizeOf_(item->GetText()); // all ok
            UI().ClientToScreenScaledWidth(tmp_len);

            if (tmp_len > len)
                len = tmp_len;
        }
    }
    return len;
}

void CUIListBox::SetImmediateSelection(bool f) { m_bImmediateSelection = f; }
