#include "pch.hpp"
#include "UIPropertiesBox.h"
#include "ListBox/UIListBoxItem.h"
#include "XML/UIXmlInitBase.h"

#define OFFSET_X (5.0f)
#define OFFSET_Y (5.0f)
#define ITEM_HEIGHT (GetFont()->CurrentHeight() + 2.0f)

CUIPropertiesBox::CUIPropertiesBox(CUIPropertiesBox* sub_property_box)
{
    m_UIListWnd.SetFont(UI().Font().pFontArial14);
    m_UIListWnd.SetImmediateSelection(true);

    m_sub_property_box = sub_property_box;
    m_parent_sub_menu = NULL;
    m_item_sub_menu_initiator = NULL;
    if (m_sub_property_box)
        m_sub_property_box->SetParentSubMenu(this);
}

CUIPropertiesBox::~CUIPropertiesBox()
{
    R_ASSERT2(!m_sub_property_box || (!m_sub_property_box->IsShown()),
        "child sub menu is in shown mode - he'll tries to hide this menu");
}

void CUIPropertiesBox::InitPropertiesBox(Fvector2 pos, Fvector2 size)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);

    AttachChild(&m_UIListWnd);

    CUIXml xml_doc;
    const bool loaded = xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "actor_menu.xml", false);

    if (!loaded || !xml_doc.NavigateToNode("properties_box")) // SOC and CS compatibility
    {
        xml_doc.ClearInternal();
        xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "inventory_new.xml");
        R_ASSERT2(xml_doc.NavigateToNode("properties_box"), "Can't find properties_box in [actor_menu.xml]");
    }

    LPCSTR t = xml_doc.Read("properties_box:texture", 0, nullptr);
    R_ASSERT2(t, "Please, specify texture for properties_box");
    InitTexture(t);

    CUIXmlInitBase::InitListBox(xml_doc, "properties_box:list", 0, &m_UIListWnd);

    m_UIListWnd.SetWndPos(Fvector2().set(OFFSET_X, OFFSET_Y));
    m_UIListWnd.SetWndSize(Fvector2().set(size.x - OFFSET_X * 2, size.y - OFFSET_Y * 2));
}

void CUIPropertiesBox::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == &m_UIListWnd)
    {
        if (msg == LIST_ITEM_CLICKED)
        {
            GetMessageTarget()->SendMessage(this, PROPERTY_CLICKED);
            if (!m_sub_property_box) // i'm the last sub menu
            {
                Hide();
                if (m_parent_sub_menu) // if i have a parent sub menu, close it
                    m_parent_sub_menu->Hide();
            }
        }
    }
    CUIWndCallback::OnEvent(pWnd, msg, pData);
    inherited::SendMessage(pWnd, msg, pData);
}

void CUIPropertiesBox::ShowSubMenu()
{
    R_ASSERT(m_sub_property_box);
    R_ASSERT(!m_sub_property_box->IsShown());
    m_item_sub_menu_initiator = GetClickedItem();

    Frect tmp_pbox_rect = m_last_show_rect;

    Fvector2 tmp_pbox_pos = GetWndPos();
    tmp_pbox_pos.y += m_item_sub_menu_initiator->GetWndPos().y + (m_item_sub_menu_initiator->GetHeight() / 2);

    float right_limit = tmp_pbox_pos.x + GetWidth() + m_sub_property_box->GetWidth();
    // show sub menu on left or right site
    if (right_limit < tmp_pbox_rect.x2)
    {
        // on right
        tmp_pbox_rect.x1 = tmp_pbox_pos.x;
        tmp_pbox_pos.x += GetWidth();
    }
    else
    {
        // on left
        tmp_pbox_rect.x2 = tmp_pbox_pos.x;
    }
    m_sub_property_box->Show(tmp_pbox_rect, tmp_pbox_pos);
}

void CUIPropertiesBox::OnItemReceivedFocus(CUIWindow* w, void* d)
{
    VERIFY(m_sub_property_box);
    if (m_sub_property_box->IsShown() && (w != m_item_sub_menu_initiator))
    {
        m_sub_property_box->Hide();
    }
}

bool CUIPropertiesBox::AddItem(LPCSTR str, void* pData, u32 tag_value)
{
    CUIListBoxItem* itm = m_UIListWnd.AddTextItem(str);
    itm->SetTAG(tag_value);
    itm->SetData(pData);
    if (m_sub_property_box)
    {
        AddCallback(
            itm, WINDOW_FOCUS_RECEIVED, CUIWndCallback::void_function(this, &CUIPropertiesBox::OnItemReceivedFocus));
        Register(itm);
    }
    return true;
}
void CUIPropertiesBox::RemoveItemByTAG(u32 tag) { m_UIListWnd.RemoveWindow(m_UIListWnd.GetItemByTAG(tag)); }
void CUIPropertiesBox::RemoveAll() { m_UIListWnd.Clear(); }
void CUIPropertiesBox::Show(const Frect& parent_rect, const Fvector2& point)
{
    Fvector2 prop_pos;
    Fvector2 prop_size = GetWndSize();
    m_last_show_rect = parent_rect;

    if (point.x - prop_size.x > parent_rect.x1 && point.y + prop_size.y < parent_rect.y2)
    {
        prop_pos.set(point.x - prop_size.x, point.y);
    }
    else if (point.x - prop_size.x > parent_rect.x1 && point.y - prop_size.y > parent_rect.y1)
    {
        prop_pos.set(point.x - prop_size.x, point.y - prop_size.y);
    }
    else if (point.x + prop_size.x < parent_rect.x2 && point.y - prop_size.y > parent_rect.y1)
    {
        prop_pos.set(point.x, point.y - prop_size.y);
    }
    else
        prop_pos.set(point.x, point.y);

    SetWndPos(prop_pos);

    inherited::Show(true);
    inherited::Enable(true);

    ResetAll();

    GetParent()->SetCapture(this, true);
    m_UIListWnd.Reset();
}

void CUIPropertiesBox::Hide()
{
    CUIWindow::Show(false);
    CUIWindow::Enable(false);

    m_pMouseCapturer = NULL;

    if (GetParent()->GetMouseCapturer() == this)
        GetParent()->SetCapture(this, false);

    if (m_sub_property_box)
        m_sub_property_box->Hide();
}

bool CUIPropertiesBox::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    bool cursor_on_box;

    if (x >= 0 && x < GetWidth() && y >= 0 && y < GetHeight())
        cursor_on_box = true;
    else
        cursor_on_box = false;

    if (mouse_action == WINDOW_LBUTTON_DOWN && !cursor_on_box)
    {
        Hide();
        return true;
    }
    if (mouse_action == WINDOW_RBUTTON_DOWN && !cursor_on_box)
    {
        Hide();
    }
    if (mouse_action == WINDOW_MOUSE_WHEEL_DOWN || mouse_action == WINDOW_MOUSE_WHEEL_UP)
        return true;

    return inherited::OnMouseAction(x, y, mouse_action);
}

void CUIPropertiesBox::AutoUpdateSize()
{
    Fvector2 sz = GetWndSize();
    sz.y = m_UIListWnd.GetItemHeight() * m_UIListWnd.GetSize() + m_UIListWnd.GetVertIndent();
    sz.x = float(m_UIListWnd.GetLongestLength() + m_UIListWnd.GetHorizIndent()) + 2;
    SetWndSize(sz);
    m_UIListWnd.SetWndSize(GetWndSize());
    m_UIListWnd.UpdateChildrenLenght();
}

CUIListBoxItem* CUIPropertiesBox::GetClickedItem() { return m_UIListWnd.GetSelectedItem(); }
void CUIPropertiesBox::Update() { inherited::Update(); }
void CUIPropertiesBox::Draw() { inherited::Draw(); }
bool CUIPropertiesBox::OnKeyboardAction(int dik, EUIMessages keyboard_action) { return true; }
