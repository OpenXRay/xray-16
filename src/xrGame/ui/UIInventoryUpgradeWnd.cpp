////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInventoryUpgradeWnd.cpp
//	Created 	: 06.10.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "Common/object_broker.h"
#include "UIInventoryUpgradeWnd.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "string_table.h"
#include "Actor.h"
#include "xrScriptEngine/script_process.hpp"
#include "Inventory.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade.h"
#include "inventory_upgrade_property.h"
#include "UIInventoryUtilities.h"
#include "UIActorMenu.h"
#include "UIItemInfo.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIHelper.h"
#include "xrUICore/ui_defs.h"
#include "Weapon.h"
#include "WeaponRPG7.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "script_game_object.h" //Alundaio

const LPCSTR g_inventory_upgrade_xml = "inventory_upgrade.xml";

CUIInventoryUpgradeWnd::Scheme::Scheme() {}
CUIInventoryUpgradeWnd::Scheme::~Scheme() { delete_data(cells); }
// =============================================================================================

CUIInventoryUpgradeWnd::CUIInventoryUpgradeWnd()
{
    // m_WeaponIconsShader = new ui_shader();
    //(*m_WeaponIconsShader)->create("hud" DELIMITER "default", "ui" DELIMITER "ui_actor_weapons");
    // m_OutfitIconsShader = new ui_shader();
    //(*m_OutfitIconsShader)->create("hud" DELIMITER "default", "ui" DELIMITER "ui_actor_armor");
    m_inv_item = nullptr;
    m_cur_upgrade_id = nullptr;
    m_current_scheme = nullptr;
    m_btn_repair = nullptr;
}

CUIInventoryUpgradeWnd::~CUIInventoryUpgradeWnd()
{
    delete_data(m_schemes);
    // xr_delete(m_WeaponIconsShader);
    // xr_delete(m_OutfitIconsShader);
    // m_WeaponIconsShader = 0;
    // m_OutfitIconsShader = 0;
}

void CUIInventoryUpgradeWnd::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, g_inventory_upgrade_xml);

    CUIXmlInit::InitWindow(uiXml, "main", 0, this);

    m_item = UIHelper::CreateStatic(uiXml, "item_static", this);
    m_back = UIHelper::CreateNormalWindow(uiXml, "back", this);
    m_scheme_wnd = UIHelper::CreateNormalWindow(uiXml, "scheme", this);
    m_btn_repair = UIHelper::Create3tButton(uiXml, "repair_button", this);

    LoadCellsBacks(uiXml);
    LoadSchemes(uiXml);
}

void CUIInventoryUpgradeWnd::InitInventory(CInventoryItem* item, bool can_upgrade)
{
    m_inv_item = item;
    bool is_shader = false;
    // Загружаем картинку
    if (smart_cast<CWeapon*>(item))
    {
        is_shader = true;
        m_item->SetShader(InventoryUtilities::GetWeaponUpgradeIconsShader());
        if (smart_cast<CWeaponRPG7*>(item))
            m_item->SetShader(InventoryUtilities::GetOutfitUpgradeIconsShader());
    }
    else if (smart_cast<CCustomOutfit*>(item) || smart_cast<CHelmet*>(item))
    {
        is_shader = true;
        m_item->SetShader(InventoryUtilities::GetOutfitUpgradeIconsShader());
    }

    if (m_item && is_shader)
    {
        Irect item_upgrade_grid_rect = item->GetUpgrIconRect();
        Frect texture_rect;
        texture_rect.lt.set(item_upgrade_grid_rect.x1, item_upgrade_grid_rect.y1);
        texture_rect.rb.set(item_upgrade_grid_rect.x2, item_upgrade_grid_rect.y2);
        texture_rect.rb.add(texture_rect.lt);
        m_item->GetUIStaticItem().SetTextureRect(texture_rect);
        m_item->TextureOn();
        m_item->SetStretchTexture(true);
        Fvector2 v_r = Fvector2().set(item_upgrade_grid_rect.x2, item_upgrade_grid_rect.y2);
        if (UI().is_widescreen())
            v_r.x *= 0.8f;

        m_item->GetUIStaticItem().SetSize(v_r);
        m_item->SetWidth(v_r.x);
        m_item->SetHeight(v_r.y);
        m_item->Show(true);
    }
    else
        m_item->Show(false);

    m_scheme_wnd->DetachAll();
    m_scheme_wnd->Show(false);
    m_back->DetachAll();
    m_back->Show(false);
    m_btn_repair->Enable(false);

    if (ai().get_alife() && m_inv_item)
    {
        if (install_item(*m_inv_item, can_upgrade))
        {
            UpdateAllUpgrades();
        }
    }
}

// ------------------------------------------------------------------------------------------

void CUIInventoryUpgradeWnd::Show(bool status)
{
    inherited::Show(status);
    UpdateAllUpgrades();
}

void CUIInventoryUpgradeWnd::Update() { inherited::Update(); }
void CUIInventoryUpgradeWnd::Reset()
{
    for (Scheme* scheme : m_schemes)
    {
        for (auto& cell : scheme->cells)
        {
            cell->Reset();
            cell->m_point->Reset();
        }
    }

    inherited::Reset();
    inherited::ResetAll();
}

void CUIInventoryUpgradeWnd::UpdateAllUpgrades()
{
    if (!m_current_scheme || !m_inv_item)
    {
        return;
    }

    for (auto& cell : m_current_scheme->cells)
        cell->update_item(m_inv_item);
}

void CUIInventoryUpgradeWnd::SetCurScheme(const shared_str& id)
{
    for (Scheme* scheme : m_schemes)
    {
        if (scheme->name._get() == id._get())
        {
            m_current_scheme = scheme;
            return;
        }
    }
    VERIFY2(0, make_string("Scheme <%s> does not loaded !", id.c_str()));
}

bool CUIInventoryUpgradeWnd::install_item(CInventoryItem& inv_item, bool can_upgrade)
{
    m_scheme_wnd->DetachAll();
    m_back->DetachAll();
    m_btn_repair->Enable((inv_item.GetCondition() < 0.99f));

    if (!can_upgrade)
    {
#ifdef DEBUG
        Msg("Inventory item <%s> cannot upgrade - Mechanic say.", inv_item.m_section_id.c_str());
#endif // DEBUG
        m_current_scheme = nullptr;
        return false;
    }

    LPCSTR scheme_name = get_manager().get_item_scheme(inv_item);
    if (!scheme_name)
    {
#ifdef DEBUG
        Msg("Inventory item <%s> does not contain upgrade scheme.", inv_item.m_section_id.c_str());
#endif // DEBUG
        m_current_scheme = nullptr;
        return false;
    }

    SetCurScheme(scheme_name);

    for (UIUpgrade* ui_item : m_current_scheme->cells)
    {
        m_scheme_wnd->AttachChild(ui_item);
        //		m_item->AttachChild( ui_item->m_point );
        m_back->AttachChild(ui_item->m_point);

        LPCSTR upgrade_name = get_manager().get_upgrade_by_index(inv_item, ui_item->get_scheme_index());
        ui_item->init_upgrade(upgrade_name, inv_item);

        Upgrade_type* upgrade_p = get_manager().get_upgrade(upgrade_name);
        VERIFY(upgrade_p);
        for (u8 i = 0; i < inventory::upgrade::max_properties_count; i++)
        {
            shared_str prop_name = upgrade_p->get_property_name(i);
            if (prop_name.size())
            {
                Property_type* prop_p = get_manager().get_property(prop_name);
                VERIFY(prop_p);
            }
        }

        ui_item->set_texture(UIUpgrade::LAYER_ITEM, upgrade_p->icon_name());
        ui_item->set_texture(UIUpgrade::LAYER_POINT, m_point_textures[UIUpgrade::STATE_ENABLED].c_str()); // default
        ui_item->set_texture(UIUpgrade::LAYER_COLOR, m_cell_textures[UIUpgrade::STATE_ENABLED].c_str()); // default
    }

    m_scheme_wnd->Show(true);
    m_item->Show(true);
    m_back->Show(true);

    UpdateAllUpgrades();
    return true;
}

UIUpgrade* CUIInventoryUpgradeWnd::FindUIUpgrade(Upgrade_type const* upgr)
{
    if (!m_current_scheme)
        return nullptr;

    for (UIUpgrade* cell : m_current_scheme->cells)
    {
        Upgrade_type* i_upgr = cell->get_upgrade();
        if (upgr == i_upgr)
        {
            return cell;
        }
    }
    return nullptr;
}

bool CUIInventoryUpgradeWnd::DBClickOnUIUpgrade(Upgrade_type const* upgr)
{
    UpdateAllUpgrades();
    UIUpgrade* uiupgr = FindUIUpgrade(upgr);
    if (uiupgr)
    {
        uiupgr->OnClick();
        return true;
    }
    return false;
}

void CUIInventoryUpgradeWnd::AskUsing(LPCSTR text, LPCSTR upgrade_name)
{
    VERIFY(m_inv_item);
    VERIFY(upgrade_name);
    VERIFY(m_pParentWnd);

    UpdateAllUpgrades();

    m_cur_upgrade_id = upgrade_name;

    CUIActorMenu* parent_wnd = smart_cast<CUIActorMenu*>(m_pParentWnd);
    if (parent_wnd)
    {
        parent_wnd->CallMessageBoxYesNo(text);
    }
}

void CUIInventoryUpgradeWnd::OnMesBoxYes()
{
    if (get_manager().upgrade_install(*m_inv_item, m_cur_upgrade_id, false))
    {
        VERIFY(m_pParentWnd);
        CUIActorMenu* parent_wnd = smart_cast<CUIActorMenu*>(m_pParentWnd);
        if (parent_wnd)
        {
            //Alundaio: tell script that item has been upgraded
            luabind::functor<void> funct;
            GEnv.ScriptEngine->functor("inventory_upgrades.effect_upgrade_item", funct);
            if (funct)
            {
                CGameObject* GO = m_inv_item->cast_game_object();
                funct(GO->lua_game_object(), m_cur_upgrade_id);
            }
            //-Alundaio
            parent_wnd->UpdateActor();
            parent_wnd->SeparateUpgradeItem();
        }
    }
    UpdateAllUpgrades();
}

void CUIInventoryUpgradeWnd::HighlightHierarchy(shared_str const& upgrade_id)
{
    UpdateAllUpgrades();
    get_manager().highlight_hierarchy(*m_inv_item, upgrade_id);
}

void CUIInventoryUpgradeWnd::ResetHighlight()
{
    UpdateAllUpgrades();
    get_manager().reset_highlight(*m_inv_item);
}

void CUIInventoryUpgradeWnd::set_info_cur_upgrade(Upgrade_type* upgrade)
{
    UIUpgrade* uiu = FindUIUpgrade(upgrade);
    if (uiu)
    {
        if (Device.dwTimeGlobal < uiu->FocusReceiveTime())
        {
            upgrade = nullptr; // visible = false
        }
    }
    else
    {
        upgrade = nullptr;
    }

    CUIActorMenu* parent_wnd = smart_cast<CUIActorMenu*>(m_pParentWnd);
    if (parent_wnd)
    {
        if (parent_wnd->SetInfoCurUpgrade(upgrade, m_inv_item))
        {
            UpdateAllUpgrades();
        }
    }
}

CUIInventoryUpgradeWnd::Manager_type& CUIInventoryUpgradeWnd::get_manager()
{
    return ai().alife().inventory_upgrade_manager();
}
