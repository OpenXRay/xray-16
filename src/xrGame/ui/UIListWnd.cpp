#include "StdAfx.h"
#include "UIListWnd.h"
//.#include "uiscrollbar.h"
#include "UIFrameLineWnd.h"

//. #define				ACTIVE_BACKGROUND			"ui" DELIMITER "ui_pop_up_active_back"
//. #define				ACTIVE_BACKGROUND_WIDTH		16
//. #define				ACTIVE_BACKGROUND_HEIGHT	16

// разделитель для интерактивных строк в листе
static const char cSeparatorChar = '%';

CUIListWnd::CUIListWnd()
{
    m_ActiveBackgroundFrame = NULL;
    m_bListActivity = true;
    m_iFocusedItem = -1;
    m_iSelectedItem = -1;
    m_iFocusedItemGroupID = -1;
    m_iSelectedItemGroupID = -1;
    m_bShowSelectedItem = false;
    m_bActiveBackground = false;
    m_dwFontColor = 0xFFFFFFFF;
    SetItemHeight(DEFAULT_ITEM_HEIGHT);
    m_bVertFlip = false;
    m_bUpdateMouseMove = false;
    m_bForceFocusedItem = false;
    m_iLastUniqueID = 0;
    m_bAlwaysShowScroll = false;
    m_bAlwaysShowScroll_enable = false;
}

CUIListWnd::~CUIListWnd()
{
    while (!m_ItemList.empty())
        DetachChild(m_ItemList.front());

    m_ItemList.clear();
    xr_delete(m_ActiveBackgroundFrame);
}

/*
void CUIListWnd::Init(float x, float y, float width, float height)
{
    Init(x, y, width, height, m_iItemHeight);
}*/

void CUIListWnd::InitListWnd(Fvector2 pos, Fvector2 size, float item_height)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);

    //добавить полосу прокрутки
    m_ScrollBar = xr_new<CUIScrollBar>();
    m_ScrollBar->SetAutoDelete(true);
    AttachChild(m_ScrollBar);

    if (!!m_scrollbar_profile)
        m_ScrollBar->InitScrollBar(Fvector2().set(size.x, 0.0f), size.y, false, *m_scrollbar_profile);
    else
        m_ScrollBar->InitScrollBar(Fvector2().set(size.x, 0.0f), size.y, false);

    m_ScrollBar->SetWndPos(
        Fvector2().set(m_ScrollBar->GetWndPos().x - m_ScrollBar->GetWidth(), m_ScrollBar->GetWndPos().y));

    SetItemWidth(size.x - m_ScrollBar->GetWidth());

    m_iFirstShownIndex = 0;

    SetItemHeight(item_height);
    m_iRowNum = iFloor(size.y / m_iItemHeight);

    m_ScrollBar->SetRange(0, 0);
    m_ScrollBar->SetPageSize(0);
    m_ScrollBar->SetScrollPos(s16(m_iFirstShownIndex));

    m_ScrollBar->Show(false);
    m_ScrollBar->Enable(false);

    /*
        m_StaticActiveBackground.Init(ACTIVE_BACKGROUND,"hud" DELIMITER "default", 0,0,alNone);
        m_StaticActiveBackground.SetTile(iFloor(m_iItemWidth/ACTIVE_BACKGROUND_WIDTH),
                                         iFloor(m_iItemHeight/ACTIVE_BACKGROUND_HEIGHT),
                                         fmod(m_iItemWidth,float(ACTIVE_BACKGROUND_WIDTH)),
                                         fmod(m_iItemHeight,float(ACTIVE_BACKGROUND_HEIGHT)));
    */
    UpdateList();
}
//////////////////////////////////////////////////////////////////////////

/*was made within plan of dinamic changing of appea*/
void CUIListWnd::SetHeight(float height)
{
    CUIWindow::SetHeight(height);
    m_iRowNum = iFloor(height / m_iItemHeight);
    m_ScrollBar->SetHeight(height);
    this->UpdateList();
    this->UpdateScrollBar();
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::SetWidth(float width)
{
    inherited::SetWidth(width);
    /*
        m_StaticActiveBackground.SetTile(iFloor(GetWidth()/ACTIVE_BACKGROUND_WIDTH),
                                         iFloor(m_iItemHeight/ACTIVE_BACKGROUND_HEIGHT),
                                         fmod(GetWidth(),float(ACTIVE_BACKGROUND_WIDTH)),
                                         fmod(float(m_iItemHeight),float(ACTIVE_BACKGROUND_HEIGHT))
                                         );
    */
}

void CUIListWnd::RemoveItem(int index)
{
    if (index < 0 || index >= (int)m_ItemList.size())
        return;

    LIST_ITEM_LIST_it it;

    //выбрать нужный элемент
    it = m_ItemList.begin();
    for (int i = 0; i < index; ++i, ++it)
        ;

    R_ASSERT(m_ItemList.end() != it);

    DetachChild(*it);

    UpdateList();

    //обновить полосу прокрутки
    if (m_ItemList.size() > 0)
        m_ScrollBar->SetRange(0, s16(m_ItemList.size() - 1));
    else
        m_ScrollBar->SetRange(0, 0);

    m_ScrollBar->SetPageSize((m_iRowNum < (int)m_ItemList.size()) ? m_iRowNum : (int)m_ItemList.size());
    m_ScrollBar->SetScrollPos(s16(m_iFirstShownIndex));
    m_ScrollBar->Refresh();

    //перенумеровать индексы заново
    i = 0;
    for (LIST_ITEM_LIST_it it = m_ItemList.begin(); m_ItemList.end() != it; ++it, i++)
    {
        (*it)->SetIndex(i);
    }
}

//////////////////////////////////////////////////////////////////////////

CUIListItem* CUIListWnd::GetItem(int index)
{
    if (index < 0 || index >= (int)m_ItemList.size())
        return NULL;

    LIST_ITEM_LIST_it it;

    //выбрать нужный элемент
    it = m_ItemList.begin();
    for (int i = 0; i < index; ++i, ++it)
        ;

    R_ASSERT(m_ItemList.end() != it);

    return (*it);
}

void CUIListWnd::DetachChild(CUIWindow* pChild)
{
    LIST_ITEM_LIST_it it = std::find(m_ItemList.begin(), m_ItemList.end(), pChild);
    if (it != m_ItemList.end())
        m_ItemList.erase(it);

    inherited::DetachChild(pChild);
}

void CUIListWnd::RemoveAll()
{
    if (m_ItemList.empty())
        return;

    LIST_ITEM_LIST_it it;

    while (!m_ItemList.empty())
    {
        DetachChild(m_ItemList.front());
    }

    m_iFirstShownIndex = 0;

    UpdateList();
    Reset();

    //обновить полосу прокрутки
    m_ScrollBar->SetRange(0, 0);
    m_ScrollBar->SetPageSize(1);
    m_ScrollBar->SetScrollPos(s16(m_iFirstShownIndex));

    UpdateScrollBar();
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::UpdateList()
{
    if (m_ItemList.empty())
    {
        UpdateScrollBar();
        return;
    }

    LIST_ITEM_LIST_it it = m_ItemList.begin();

    //спрятать все элементы до участка
    //отображающейся в данный момент
    for (int i = 0; i < _min(m_ItemList.size(), m_iFirstShownIndex); ++i, ++it)
    {
        (*it)->Show(false);
    }

    //показать текущий список
    for (i = m_iFirstShownIndex; i < _min(m_ItemList.size(), m_iFirstShownIndex + m_iRowNum + 1); ++i, ++it)
    {
        Frect rect_to_set;
        rect_to_set.lt.set((*it)->GetWndRect().left, m_bVertFlip ?
                GetHeight() - (i - m_iFirstShownIndex) * m_iItemHeight - m_iItemHeight :
                (i - m_iFirstShownIndex) * m_iItemHeight);
        rect_to_set.rb.add(rect_to_set.lt, Fvector2().set(m_iItemWidth, m_iItemHeight));

        (*it)->SetWndRect(rect_to_set);
        (*it)->Show(true);

        if (m_bListActivity)
            (*it)->Enable(true);
        else
            (*it)->Enable(false);
    }

    --it;

    //спрятать все после
    for (u32 k = m_iFirstShownIndex + m_iRowNum; k < m_ItemList.size(); ++k, ++it)
    {
        (*it)->Show(false);
        //		(*it)->Enable(false);
    }

    UpdateScrollBar();
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_ScrollBar)
    {
        if (msg == SCROLLBAR_VSCROLL)
        {
            m_iFirstShownIndex = m_ScrollBar->GetScrollPos();
            UpdateList();
            GetMessageTarget()->SendMessage(this, SCROLLBAR_VSCROLL, NULL);
        }
    }
    else
    {
        //если сообщение пришло от одного из элементов списка

        if (IsChild(pWnd))
        {
            CUIListItem* pListItem2;
            CUIListItem* pListItem = smart_cast<CUIListItem*>(pWnd);
            R_ASSERT(pListItem);

            if (BUTTON_CLICKED == msg)
            {
                for (WINDOW_LIST_it it = m_ChildWndList.begin(); it != m_ChildWndList.end(); ++it)
                {
                    pListItem2 = smart_cast<CUIListItem*>(*it);
                    if (!pListItem2)
                        continue;
                    if (pListItem2->GetGroupID() == -1)
                        continue;
                    if (pListItem2->GetGroupID() == pListItem->GetGroupID())
                    {
                        pListItem2->SetHighlightText(true);
                        pListItem2->SendMessage(this, LIST_ITEM_SELECT, pData);
                        m_iSelectedItem = pListItem2->GetIndex();
                        m_iSelectedItemGroupID = pListItem2->GetGroupID();
                    }
                    else
                    {
                        pListItem2->SetHighlightText(false);
                        pListItem2->SendMessage(this, LIST_ITEM_UNSELECT, pData);
                    }
                }
                GetMessageTarget()->SendMessage(this, LIST_ITEM_CLICKED, pListItem);
            }

            else if (STATIC_FOCUS_RECEIVED == msg)
            {
                if (!m_bForceFocusedItem)
                {
                    m_iFocusedItem = pListItem->GetIndex();
                    m_iFocusedItemGroupID = pListItem->GetGroupID();
                }
                else if (m_iFocusedItem >= 0)
                    m_iFocusedItemGroupID = GetItem(m_iFocusedItem)->GetGroupID();

                // prototype code

                for (WINDOW_LIST_it it = m_ChildWndList.begin(); it != m_ChildWndList.end(); ++it)
                {
                    pListItem2 = smart_cast<CUIListItem*>(*it);
                    if (!pListItem2)
                        continue;
                    if (pListItem2->GetGroupID() == -1)
                        continue;
                    if (pListItem2->GetGroupID() == pListItem->GetGroupID())
                    {
                        pListItem2->SetHighlightText(true);
                        pListItem2->SendMessage(this, STATIC_FOCUS_RECEIVED, pData);
                    }
                    else
                    {
                        pListItem2->SetHighlightText(false);
                        pListItem2->SendMessage(this, STATIC_FOCUS_LOST, pData);
                    }
                }
                // end prototype code
            }
            else if (STATIC_FOCUS_LOST == msg)
            {
                if (pListItem->GetIndex() == m_iFocusedItem && !m_bForceFocusedItem)
                    m_iFocusedItem = -1;

                for (WINDOW_LIST_it it = m_ChildWndList.begin(); it != m_ChildWndList.end(); ++it)
                {
                    pListItem2 = smart_cast<CUIListItem*>(*it);
                    if (!pListItem2)
                        continue;
                    pListItem2->SetHighlightText(false);
                    pListItem2->SendMessage(this, STATIC_FOCUS_LOST, pData);
                }
                m_bUpdateMouseMove = true;
            }
        }
    }

    CUIWindow::SendMessage(pWnd, msg, pData);
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::DrawActiveBackFrame(const Frect& rect, CUIListItem* itm)
{
    Fvector2 _pos;
    _pos.set(rect.left, rect.top + (itm->GetIndex() - m_iFirstShownIndex) * GetItemHeight());
    float _d = GetItemHeight() - m_ActiveBackgroundFrame->GetHeight();
    if (_d > 0)
        _pos.y += (float)iFloor(_d / 2.0f);

    m_ActiveBackgroundFrame->SetWndPos(_pos);
    float _w = GetWidth();
    if (m_ScrollBar->IsShown())
        _w -= m_ScrollBar->GetWidth();
    m_ActiveBackgroundFrame->SetWidth(_w);
    m_ActiveBackgroundFrame->Draw();
}

void CUIListWnd::Draw()
{
    WINDOW_LIST_it it;

    if (m_iFocusedItem != -1 && IsActiveBackgroundEnabled())
    {
        Frect rect;
        GetAbsoluteRect(rect);
        for (it = m_ChildWndList.begin(); it != m_ChildWndList.end(); ++it)
        {
            CUIListItem* pListItem2 = smart_cast<CUIListItem*>(*it);
            if (!pListItem2)
                continue;
            if (pListItem2->GetGroupID() == -1)
                continue;
            if ((pListItem2->GetGroupID() == m_iFocusedItemGroupID) &&
                ((pListItem2->GetIndex() >= m_iFirstShownIndex) &&
                    (pListItem2->GetIndex() <= m_iRowNum + m_iFirstShownIndex - 1)))
            {
                DrawActiveBackFrame(rect, pListItem2);
            }
        }
    }

    if (m_iSelectedItem != -1 && m_bShowSelectedItem)
    {
        Frect rect;
        GetAbsoluteRect(rect);
        for (it = m_ChildWndList.begin(); it != m_ChildWndList.end(); ++it)
        {
            CUIListItem* pListItem2 = smart_cast<CUIListItem*>(*it);
            if (!pListItem2)
                continue;
            if (pListItem2->GetGroupID() == -1)
                continue;
            if (pListItem2->GetIndex() == m_iSelectedItem)
            {
                UI()->PushScissor(rect);

                DrawActiveBackFrame(rect, pListItem2);

                UI()->PopScissor();
            }
        }
    }

    CUIWindow::Draw();
}

void CUIListWnd::SetItemWidth(float iItemWidth) { m_iItemWidth = iItemWidth; }
//////////////////////////////////////////////////////////////////////////

void CUIListWnd::SetItemHeight(float iItemHeight)
{
    m_iItemHeight = iItemHeight;
    m_iRowNum = iFloor(GetHeight() / iItemHeight);
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::Reset()
{
    for (LIST_ITEM_LIST_it it = m_ItemList.begin(); m_ItemList.end() != it; ++it)
    {
        (*it)->Reset();
    }

    ResetAll();

    inherited::Reset();
}

//////////////////////////////////////////////////////////////////////////
//находит первый элемент с заданной pData, иначе -1
//////////////////////////////////////////////////////////////////////////

int CUIListWnd::FindItem(void* pData)
{
    int i = 0;
    for (LIST_ITEM_LIST_it it = m_ItemList.begin(); m_ItemList.end() != it; ++it, ++i)
    {
        if ((*it)->GetData() == pData)
            return i;
    }
    return -1;
}

int CUIListWnd::FindItemWithValue(int iValue)
{
    int i = 0;
    for (LIST_ITEM_LIST_it it = m_ItemList.begin(); m_ItemList.end() != it; ++it, ++i)
    {
        if ((*it)->GetValue() == iValue)
            return i;
    }
    return -1;
}

bool CUIListWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
    switch (mouse_action)
    {
    case WINDOW_LBUTTON_DB_CLICK: break;
    case WINDOW_MOUSE_WHEEL_DOWN:
        m_ScrollBar->TryScrollInc();
        return true;
        break;
    case WINDOW_MOUSE_WHEEL_UP:
        m_ScrollBar->TryScrollDec();
        return true;
        break;
    }

    return inherited::OnMouse(x, y, mouse_action);
}

bool CUIListWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
    return inherited::OnKeyboard(dik, keyboard_action);
}
//////////////////////////////////////////////////////////////////////////

void CUIListWnd::UpdateScrollBar()
{
    if (m_bAlwaysShowScroll_enable)
    {
        m_ScrollBar->Show(m_bAlwaysShowScroll);
        return;
    }

    if ((int)m_ItemList.size() <= m_ScrollBar->GetPageSize())
        m_ScrollBar->Show(false);
    else
        m_ScrollBar->Show(true);
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::EnableScrollBar(bool enable)
{
    m_ScrollBar->SetEnabled(enable);
    UpdateScrollBar();
}

void CUIListWnd::ActivateList(bool activity) { m_bListActivity = activity; }
//////////////////////////////////////////////////////////////////////////

void CUIListWnd::ScrollToBegin()
{
    m_ScrollBar->SetScrollPos((s16)m_ScrollBar->GetMinRange());
    m_iFirstShownIndex = m_ScrollBar->GetScrollPos();
    UpdateList();
}

//////////////////////////////////////////////////////////////////////////

void CUIListWnd::ScrollToEnd()
{
    u32 pos = m_ScrollBar->GetMaxRange() - m_ScrollBar->GetPageSize() + 1;

    if ((int)pos > m_ScrollBar->GetMinRange())
        m_ScrollBar->SetScrollPos(pos);
    else
        m_ScrollBar->SetScrollPos(m_ScrollBar->GetMinRange());

    m_iFirstShownIndex = m_ScrollBar->GetScrollPos();
    UpdateList();
}

void CUIListWnd::ScrollToPos(int position)
{
    if (IsScrollBarEnabled())
    {
        int pos = position;
        clamp(pos, m_ScrollBar->GetMinRange(), (m_ScrollBar->GetMaxRange() - m_ScrollBar->GetPageSize() / +1));
        m_ScrollBar->SetScrollPos(pos);
        m_iFirstShownIndex = m_ScrollBar->GetScrollPos();
        UpdateList();
    }
}

void CUIListWnd::Update()
{
    if (m_bUpdateMouseMove)
    {
        OnMouse(cursor_pos.x, cursor_pos.y, WINDOW_MOUSE_MOVE);
        m_bUpdateMouseMove = false;
    }

    inherited::Update();
    if (m_ActiveBackgroundFrame)
        m_ActiveBackgroundFrame->Update();
}

void CUIListWnd::SetFocusedItem(int iNewFocusedItem)
{
    m_iFocusedItem = iNewFocusedItem;
    m_bForceFocusedItem = true;
    EnableActiveBackground(true);
    if (m_iFocusedItem >= 0)
        m_iFocusedItemGroupID = GetItem(m_iFocusedItem)->GetGroupID();
}

int CUIListWnd::GetItemPos(CUIListItem* pItem)
{
    LIST_ITEM_LIST_it it = m_ItemList.begin();
    for (u32 i = 0; i < m_ItemList.size(); ++i)
    {
        if (*it == pItem)
            return i;
        ++it;
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////

bool CUIListWnd::IsScrollBarEnabled() { return m_ScrollBar->GetEnabled(); }
void CUIListWnd::EnableActiveBackground(bool enable)
{
    m_bActiveBackground = enable;

    if (enable)
    {
        create_active_back();
    }
    else
        destroy_active_back();
}

void CUIListWnd::ShowSelectedItem(bool show)
{
    m_bShowSelectedItem = show;

    if (show)
    {
        create_active_back();
    }
    else
        destroy_active_back();
}

void CUIListWnd::create_active_back()
{
    if (m_ActiveBackgroundFrame)
        return;

    m_ActiveBackgroundFrame = xr_new<CUIFrameLineWnd>();
    m_ActiveBackgroundFrame->InitFrameLineWnd(
        "ui_listline", Fvector2().set(0.0f, 0.0f), Fvector2().set(GetWidth(), 18.0f));
}

void CUIListWnd::destroy_active_back()
{
    if (!m_bShowSelectedItem && !m_bActiveBackground)
        xr_delete(m_ActiveBackgroundFrame);
}
