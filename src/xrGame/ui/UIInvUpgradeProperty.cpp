////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInvUpgradeProperty.cpp
//	Created 	: 22.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade property UIWindow class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIInvUpgradeProperty.h"
#include "UIInvUpgradeInfo.h"

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade.h"
#include "inventory_upgrade_property.h"

UIProperty::UIProperty()
{
    m_text[0] = 0;
    m_ui_icon = NULL;
    m_ui_text = NULL;
}

UIProperty::~UIProperty() {}
void UIProperty::init_from_xml(CUIXml& ui_xml)
{
    m_ui_icon = new CUIStatic();
    m_ui_text = new CUITextWnd();
    AttachChild(m_ui_icon);
    AttachChild(m_ui_text);
    m_ui_icon->SetAutoDelete(true);
    m_ui_text->SetAutoDelete(true);

    CUIXmlInit::InitWindow(ui_xml, "properties", 0, this);
    SetWndPos(Fvector2().set(0, 0));
    CUIXmlInit::InitStatic(ui_xml, "properties:icon", 0, m_ui_icon);
    CUIXmlInit::InitTextWnd(ui_xml, "properties:text", 0, m_ui_text);
}

bool UIProperty::init_property(shared_str const& property_id)
{
    m_property_id = property_id;
    if (!get_property())
    {
        return false;
    }
    m_ui_icon->InitTexture(get_property()->icon_name());
    return true;
}

UIProperty::Property_type* UIProperty::get_property()
{
    if (!ai().get_alife())
    {
        return NULL;
    }
    Property_type* proper = ai().alife().inventory_upgrade_manager().get_property(m_property_id);
    VERIFY(proper);
    return proper;
}

bool UIProperty::read_value_from_section(LPCSTR section, LPCSTR param, float& result)
{
    result = 0.0f;
    if (!section || !pSettings->section_exist(section))
    {
        return false;
    }

    if (pSettings->line_exist(section, param) && *pSettings->r_string(section, param))
    {
        result = pSettings->r_float(section, param);
        return true;
    }
    return false;
}

bool UIProperty::compute_value(ItemUpgrades_type const& item_upgrades)
{
    if (!get_property())
    {
        return false;
    }

    int prop_count = 0;
    string2048 buf;
    buf[0] = 0;
    ItemUpgrades_type::const_iterator ib_upg = item_upgrades.begin();
    ItemUpgrades_type::const_iterator ie_upg = item_upgrades.end();
    for (; ib_upg != ie_upg; ++ib_upg)
    {
        Upgrade_type* upgr = ai().alife().inventory_upgrade_manager().get_upgrade(*ib_upg);
        VERIFY(upgr);
        for (u8 i = 0; i < inventory::upgrade::max_properties_count; i++)
        {
            if (upgr->get_property_name(i)._get() == m_property_id._get())
            {
                LPCSTR upgr_section = upgr->section();
                if (prop_count > 0)
                {
                    xr_strcat(buf, sizeof(buf), ", ");
                }
                xr_strcat(buf, sizeof(buf), upgr_section);
                ++prop_count;
            }
        }
    }
    if (prop_count > 0)
    {
        return show_result(buf);
    }
    return false;
}

bool UIProperty::show_result(LPCSTR values)
{
    if (get_property() && get_property()->run_functor(values, m_text))
    {
        m_ui_text->SetText(m_text);
        return true;
    }
    else
    {
        m_ui_text->SetText("");
        return false;
    }
}

// =================== UIPropertiesWnd =====================================================

UIInvUpgPropertiesWnd::UIInvUpgPropertiesWnd()
{
    m_properties_ui.reserve(15);
    m_temp_upgrade_vector.reserve(1);
}

UIInvUpgPropertiesWnd::~UIInvUpgPropertiesWnd() { delete_data(m_properties_ui); }
void UIInvUpgPropertiesWnd::init_from_xml(LPCSTR xml_name)
{
    CUIXml ui_xml;
    ui_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name);

    XML_NODE stored_root = ui_xml.GetLocalRoot();
    XML_NODE node = ui_xml.NavigateToNode("upgrade_info", 0);
    ui_xml.SetLocalRoot(node);

    CUIXmlInit::InitWindow(ui_xml, "properties", 0, this);

    m_Upgr_line = new CUIStatic();
    AttachChild(m_Upgr_line);
    m_Upgr_line->SetAutoDelete(true);
    CUIXmlInit::InitStatic(ui_xml, "properties:upgr_line", 0, m_Upgr_line);

    LPCSTR properties_section = "upgrades_properties";

    VERIFY2(
        pSettings->section_exist(properties_section), make_string("Section [%s] does not exist !", properties_section));
    VERIFY2(pSettings->line_count(properties_section), make_string("Section [%s] is empty !", properties_section));
    shared_str property_id;

    CInifile::Sect& inv_section = pSettings->r_section(properties_section);
    auto ib = inv_section.Data.begin();
    auto ie = inv_section.Data.end();
    for (; ib != ie; ++ib)
    {
        UIProperty* ui_property = new UIProperty(); // load one time !!
        ui_property->init_from_xml(ui_xml);

        property_id._set((*ib).first);
        if (!ui_property->init_property(property_id))
        {
            Msg("! Invalid property <%s> in inventory upgrade manager!", property_id.c_str());
            continue;
        }

        m_properties_ui.push_back(ui_property);
        AttachChild(ui_property);
    } // for ib
    ui_xml.SetLocalRoot(stored_root);
}

void UIInvUpgPropertiesWnd::set_info(ItemUpgrades_type const& item_upgrades)
{
    Fvector2 new_size;
    new_size.x = GetWndSize().x;
    new_size.y = m_Upgr_line->GetWndSize().y + 3.0f;

    Properties_type::iterator ib = m_properties_ui.begin();
    Properties_type::iterator ie = m_properties_ui.end();
    for (; ib != ie; ++ib)
    {
        UIProperty* ui_property = (*ib);
        ui_property->Show(false);

        if (ui_property->compute_value(item_upgrades))
        {
            ui_property->SetWndPos(Fvector2().set(ui_property->GetWndPos().x, new_size.y));
            new_size.y += ui_property->GetWndSize().y;
            ui_property->Show(true);
        }
    }
    new_size.y += 10.0f;
    SetWndSize(new_size);
}

void UIInvUpgPropertiesWnd::set_upgrade_info(Upgrade_type& upgrade)
{
    if (!upgrade.is_known())
    {
        SetWndSize(Fvector2().set(0, 0));
        return;
    }

    m_temp_upgrade_vector.clear();
    m_temp_upgrade_vector.push_back(upgrade.id());
    set_info(m_temp_upgrade_vector);
}

void UIInvUpgPropertiesWnd::set_item_info(CInventoryItem& item) { set_info(item.upgardes()); }
