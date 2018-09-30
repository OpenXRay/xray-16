////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInvUpgrade.cpp
//	Created 	: 08.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "Common/object_broker.h"
#include "string_table.h"

#include "UIInvUpgrade.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade.h"

#include "UIInventoryUpgradeWnd.h"

UIUpgrade::UIUpgrade(CUIInventoryUpgradeWnd* parent_wnd) : m_point(NULL)
{
    VERIFY(parent_wnd);
    m_parent_wnd = parent_wnd;

    m_item = new CUIStatic();
    m_item->SetAutoDelete(true);
    AttachChild(m_item);
    m_color = new CUIStatic();
    m_color->SetAutoDelete(true);
    AttachChild(m_color);

    m_upgrade_id = NULL;
    Reset();
}

UIUpgrade::~UIUpgrade() { xr_delete(m_point); }
void UIUpgrade::init_upgrade(LPCSTR upgrade_id, CInventoryItem& item)
{
    VERIFY(upgrade_id && xr_strcmp(upgrade_id, ""));
    m_upgrade_id = upgrade_id;

    m_prev_state = STATE_COUNT; // no defined
    update_item(&item);
}

UIUpgrade::Upgrade_type* UIUpgrade::get_upgrade()
{
    Upgrade_type* res = ai().alife().inventory_upgrade_manager().get_upgrade(m_upgrade_id);
    VERIFY(res);
    return res;
}

void UIUpgrade::Reset()
{
    offset.set(0.0f, 0.0f);

    m_prev_state = STATE_UNKNOWN;
    m_state = STATE_ENABLED;
    m_button_state = BUTTON_FREE;
    m_state_lock = false;

    m_color->Show(false);

    inherited::Reset();
}
// -----------------------------------------------------------------------------------
void UIUpgrade::load_from_xml(CUIXml& ui_xml, int i_column, int i_cell, Frect const& t_cell_item)
{
    m_scheme_index.x = i_column;
    m_scheme_index.y = i_cell; // row

    CUIXmlInit::InitWindow(ui_xml, "cell", i_cell, this);

    Fvector2 f2, color;

    f2.set(t_cell_item.x1, t_cell_item.y1);
    m_item->SetWndPos(f2);
    color.set(f2.x + (UI().is_widescreen() ? 2.0f : 3.0f), f2.y + 3.0f);
    m_color->SetWndPos(color);

    f2.set(t_cell_item.width(), t_cell_item.height());
    m_item->SetWndSize(f2);
    color.set(UI().is_widescreen() ? 4.0f : 5.0f, 38.0f);
    m_color->SetWndSize(Fvector2().set(5.0f, 38.0f));
    SetWndSize(f2);

    m_item->SetStretchTexture(true);
    m_color->SetStretchTexture(true);
}

void UIUpgrade::set_texture(Layer layer, LPCSTR texture)
{
    switch (layer)
    {
    case LAYER_ITEM:
        VERIFY(texture);
        m_item->InitTexture(texture);
        break;
    case LAYER_POINT:
        VERIFY(texture);
        m_point->InitTexture(texture);
        break;
    case LAYER_COLOR:
    {
        if (texture)
        {
            m_color->InitTexture(texture);
            m_color->Show(true);
        }
        else
        {
            m_color->Show(false);
        }
        break;
    }
    default: NODEFAULT;
    }
}

void UIUpgrade::Draw()
{
    if (get_upgrade())
        inherited::Draw();
}

void UIUpgrade::Update()
{
    inherited::Update();

    update_upgrade_state();

    if (m_prev_state != m_state)
    {
        update_mask();
    }

    m_point->Show(get_upgrade()->get_highlight());
}

void UIUpgrade::update_upgrade_state()
{
    if (m_bCursorOverWindow || m_point->CursorOverWindow())
    {
        on_over_window();
    }
    else
    {
        m_button_state = BUTTON_FREE;
    }

    if (m_state_lock)
    {
        return;
    }

    switch (m_button_state)
    {
    case BUTTON_FREE:
        if (m_state == STATE_ENABLED || m_state == STATE_FOCUSED)
            m_state = STATE_ENABLED;
        else
            m_state = STATE_DISABLED_FOCUSED;

        break;
    case BUTTON_FOCUSED:
        if (m_state == STATE_ENABLED || m_state == STATE_FOCUSED)
            m_state = STATE_FOCUSED;
        else
            m_state = STATE_DISABLED_FOCUSED;
        break;
    case BUTTON_PRESSED:
    case BUTTON_DPRESSED:
        if (m_state == STATE_ENABLED || m_state == STATE_FOCUSED)
            m_state = STATE_TOUCHED;
        break;
    }
}

void UIUpgrade::update_mask()
{
    if (m_state < STATE_ENABLED || STATE_COUNT <= m_state)
    {
        R_ASSERT2(0, "Unknown state UIUpgrade!");
    }

    set_texture(LAYER_COLOR, m_parent_wnd->get_cell_texture(m_state));
    set_texture(LAYER_POINT, m_parent_wnd->get_point_texture(m_state));
    m_prev_state = m_state;
}

bool UIUpgrade::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    if (inherited::OnMouseAction(x, y, mouse_action))
        return true;

    if (m_bCursorOverWindow || m_point->CursorOverWindow())
    {
        highlight_relation(true);
        if (mouse_action == WINDOW_LBUTTON_DOWN)
        {
            OnClick();
            return true;
        }
        if (mouse_action == WINDOW_LBUTTON_DB_CLICK)
        {
            OnDbClick();
            return true;
        }
        if (mouse_action == WINDOW_RBUTTON_DOWN)
        {
            OnRClick();
            return true;
        }
    } // m_bCursorOverWindow

    if (mouse_action == WINDOW_LBUTTON_UP || mouse_action == WINDOW_RBUTTON_UP)
    {
        m_button_state = BUTTON_FREE;
        return true;
    }

    return false;
}

void UIUpgrade::OnFocusReceive()
{
    inherited::OnFocusReceive();
    update_mask();
    highlight_relation(true);
}

void UIUpgrade::OnFocusLost()
{
    inherited::OnFocusLost();
    highlight_relation(false);

    m_parent_wnd->set_info_cur_upgrade(NULL);
    m_button_state = BUTTON_FREE;
}

void UIUpgrade::OnClick()
{
    if (m_state == STATE_ENABLED || m_state == STATE_FOCUSED || m_state == STATE_TOUCHED)
    {
        m_parent_wnd->AskUsing(
            make_string("%s %s", StringTable().translate("st_upgrade_install").c_str(), get_upgrade()->name()).c_str(),
            get_upgrade()->id_str());
    }
    m_parent_wnd->set_info_cur_upgrade(NULL);
    highlight_relation(true);

    m_button_state = BUTTON_PRESSED;
}

bool UIUpgrade::OnDbClick()
{
    m_parent_wnd->set_info_cur_upgrade(NULL);
    m_button_state = BUTTON_DPRESSED;
    return true;
}

void UIUpgrade::OnRClick()
{
    m_parent_wnd->set_info_cur_upgrade(NULL);
    highlight_relation(true);
    m_button_state = BUTTON_PRESSED;
}

void UIUpgrade::on_over_window()
{
    if (m_button_state == BUTTON_PRESSED)
    {
        return;
    }

    m_button_state = BUTTON_FOCUSED;
    m_parent_wnd->set_info_cur_upgrade(get_upgrade());
}

void UIUpgrade::highlight_relation(bool enable)
{
    if (enable)
    {
        m_parent_wnd->HighlightHierarchy(get_upgrade()->id());
        return;
    }
    m_parent_wnd->ResetHighlight();
}

void UIUpgrade::update_item(CInventoryItem* inv_item)
{
    if (!inv_item)
    {
        return;
    }
    VERIFY(get_upgrade());
    VERIFY(inv_item->m_section_id.size());

    inventory::upgrade::UpgradeStateResult res = get_upgrade()->can_install(*inv_item, false);

    m_item->SetTextureColor(color_rgba(100, 100, 100, 255));
    switch (res)
    {
    case inventory::upgrade::result_ok:
        m_item->SetTextureColor(color_rgba(255, 255, 255, 255));
        m_state = STATE_ENABLED;
        m_state_lock = false;
        break;
    case inventory::upgrade::result_e_unknown:
        m_state = STATE_UNKNOWN;
        m_state_lock = true;
        break;
    case inventory::upgrade::result_e_installed: // has_upgrade
        m_item->SetTextureColor(color_rgba(255, 255, 255, 255));
        m_state = STATE_SELECTED;
        m_state_lock = true;
        break;
    case inventory::upgrade::result_e_parents:
        m_state = STATE_DISABLED_PARENT;
        m_state_lock = false;
        break;
    case inventory::upgrade::result_e_group:
        m_item->SetTextureColor(color_rgba(255, 255, 255, 255));
        m_state = STATE_DISABLED_GROUP;
        m_state_lock = true;
        break;
    case inventory::upgrade::result_e_precondition_money:
        m_state = STATE_DISABLED_PREC_MONEY;
        m_state_lock = false;
        break;
    case inventory::upgrade::result_e_precondition_quest:
        m_state = STATE_DISABLED_PREC_QUEST;
        m_state_lock = false;
        break;

    default: NODEFAULT; break;
    }
}
void UIUpgrade::attach_point(CUIUpgradePoint* point)
{
    VERIFY(point);
    m_point = point;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////CUIUpgradePoint//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
CUIUpgradePoint::CUIUpgradePoint(UIUpgrade* upgr)
{
    VERIFY(upgr);
    m_parent_upgrade = upgr;
}

CUIUpgradePoint::~CUIUpgradePoint() {}
void CUIUpgradePoint::load_from_xml(CUIXml& ui_xml, int i_cell)
{
    float point_x = ui_xml.ReadAttribFlt("cell", i_cell, "point_x", 0.0f);
    float point_y = ui_xml.ReadAttribFlt("cell", i_cell, "point_y", 0.0f);
    SetWndPos(Fvector2().set(point_x * (UI().is_widescreen() ? 0.8f : 1.0f), point_y));
    SetWndSize(Fvector2().set(UI().is_widescreen() ? 11.0f : 14.0f, 14.0f));
    SetStretchTexture(true);
    Show(false);
}
bool CUIUpgradePoint::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    if (inherited::OnMouseAction(x, y, mouse_action))
        return true;
    if (m_bCursorOverWindow)
    {
        m_parent_upgrade->highlight_relation(true);
        if (mouse_action == WINDOW_LBUTTON_DOWN)
        {
            m_parent_upgrade->OnClick();
            return true;
        }
        if (mouse_action == WINDOW_LBUTTON_DB_CLICK)
        {
            m_parent_upgrade->OnDbClick();
            return true;
        }
        if (mouse_action == WINDOW_RBUTTON_DOWN)
        {
            m_parent_upgrade->OnRClick();
            return true;
        }
    }
    if (mouse_action == WINDOW_LBUTTON_UP || mouse_action == WINDOW_RBUTTON_UP)
    {
        m_parent_upgrade->set_button_state(UIUpgrade::BUTTON_FREE);
        return true;
    }
    return false;
}
void CUIUpgradePoint::OnFocusReceive()
{
    inherited::OnFocusReceive();
    m_parent_upgrade->set_button_state(UIUpgrade::BUTTON_FOCUSED);
    m_parent_upgrade->get_upgrade_window()->set_info_cur_upgrade(m_parent_upgrade->get_upgrade());
    m_parent_upgrade->highlight_relation(true);
    m_parent_upgrade->update_mask();
}

void CUIUpgradePoint::OnFocusLost()
{
    inherited::OnFocusLost();
    m_parent_upgrade->set_button_state(UIUpgrade::BUTTON_FREE);
    m_parent_upgrade->get_upgrade_window()->set_info_cur_upgrade(NULL);
    m_parent_upgrade->highlight_relation(false);
    m_parent_upgrade->update_mask();
}
