////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInvUpgradeInfo.cpp
//	Created 	: 21.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI info window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIInvUpgradeInfo.h"
#include "string_table.h"
#include "Actor.h"

#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"
#include "xrUICore/Windows/UIFrameWindow.h"

#include "UIInvUpgradeProperty.h"

#include "inventory_upgrade.h"
#include "inventory_upgrade_property.h"

UIInvUpgradeInfo::UIInvUpgradeInfo()
{
    m_upgrade = NULL;
    m_background = NULL;
    m_name = NULL;
    m_desc = NULL;
    m_prereq = NULL;
    m_properties_wnd = NULL;
}

UIInvUpgradeInfo::~UIInvUpgradeInfo() {}
void UIInvUpgradeInfo::init_from_xml(LPCSTR xml_name)
{
    CUIXml ui_xml;
    ui_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name);
    CUIXmlInit xml_init;

    XML_NODE stored_root = ui_xml.GetLocalRoot();
    XML_NODE node = ui_xml.NavigateToNode("upgrade_info", 0);
    ui_xml.SetLocalRoot(node);

    xml_init.InitWindow(ui_xml, "main_frame", 0, this);

    m_background = new CUIFrameWindow();
    AttachChild(m_background);
    m_background->SetAutoDelete(true);
    xml_init.InitFrameWindow(ui_xml, "background_frame", 0, m_background);

    m_name = new CUITextWnd();
    AttachChild(m_name);
    m_name->SetAutoDelete(true);
    xml_init.InitTextWnd(ui_xml, "info_name", 0, m_name);

    m_cost = new CUITextWnd();
    AttachChild(m_cost);
    m_cost->SetAutoDelete(true);
    xml_init.InitTextWnd(ui_xml, "info_cost", 0, m_cost);

    m_desc = new CUITextWnd();
    AttachChild(m_desc);
    m_desc->SetAutoDelete(true);
    xml_init.InitTextWnd(ui_xml, "info_desc", 0, m_desc);

    m_prereq = new CUITextWnd();
    AttachChild(m_prereq);
    m_prereq->SetAutoDelete(true);
    xml_init.InitTextWnd(ui_xml, "info_prerequisites", 0, m_prereq);

    m_properties_wnd = new UIInvUpgPropertiesWnd();
    AttachChild(m_properties_wnd);
    m_properties_wnd->SetAutoDelete(true);
    m_properties_wnd->init_from_xml(xml_name);

    m_properties_wnd->Show(false);
    ui_xml.SetLocalRoot(stored_root);
}

bool UIInvUpgradeInfo::init_upgrade(Upgrade_type* upgr, CInventoryItem* inv_item)
{
    if (!upgr || !inv_item)
    {
        m_upgrade = NULL;
        Show(false);
        return false;
    }

    if (m_upgrade == upgr)
    {
        return false;
    }
    m_upgrade = upgr;

    Show(true);
    m_name->Show(true);
    m_desc->Show(true);

    m_name->SetText(m_upgrade->name());
    m_desc->SetText(m_upgrade->description_text());
    if (m_upgrade->is_known())
    {
        m_prereq->Show(true);
        m_properties_wnd->Show(true);
        luabind::functor<LPCSTR> cost_func;
        LPCSTR cost_func_str = "inventory_upgrades.get_upgrade_cost";
        R_ASSERT2(GEnv.ScriptEngine->functor(cost_func_str, cost_func), "Failed to get cost");
        m_cost->SetText(cost_func(m_upgrade->section()));
        m_cost->Show(true);

        inventory::upgrade::UpgradeStateResult upg_res = m_upgrade->can_install(*inv_item, false);
        inventory::upgrade::UpgradeStateResult upg_res_script = m_upgrade->get_preconditions();
        string512 str_res = "";
        m_prereq->SetTextColor(color_rgba(255, 90, 90, 255));
        if (upg_res == inventory::upgrade::result_e_installed)
        {
            m_prereq->SetTextColor(color_rgba(117, 255, 123, 255));
            xr_sprintf(str_res, sizeof(str_res), "%s", StringTable().translate("st_upgr_installed").c_str());
        }
        else if (upg_res == inventory::upgrade::result_e_unknown)
        {
            xr_sprintf(str_res, sizeof(str_res), "%s:\\n - %s", StringTable().translate("st_upgr_disable").c_str(),
                StringTable().translate("st_upgr_unknown").c_str());
            m_cost->Show(false);
        }
        else if (upg_res == inventory::upgrade::result_e_group)
            xr_sprintf(str_res, sizeof(str_res), "%s:\\n - %s", StringTable().translate("st_upgr_disable").c_str(),
                StringTable().translate("st_upgr_group").c_str());
        else if (upg_res_script == inventory::upgrade::result_e_precondition_money)
            xr_sprintf(str_res, sizeof(str_res), "%s:\\n - %s", StringTable().translate("st_upgr_disable").c_str(),
                StringTable().translate("st_upgr_cant_do").c_str());
        else
        {
            if (upg_res != inventory::upgrade::result_ok)
            {
                xr_sprintf(str_res, sizeof(str_res), "%s:\\n%s", StringTable().translate("st_upgr_disable").c_str(),
                    m_upgrade->get_prerequisites());
                if (upg_res == inventory::upgrade::result_e_parents)
                    xr_sprintf(str_res, sizeof(str_res), "%s\\n - %s", str_res,
                        StringTable().translate("st_upgr_parents").c_str());

                if (upg_res == inventory::upgrade::result_e_precondition_money)
                    xr_sprintf(str_res, sizeof(str_res), "%s:\\n - %s",
                        StringTable().translate("st_upgr_disable").c_str(),
                        StringTable().translate("st_upgr_cant_do").c_str());
            }
        }
        m_prereq->SetText(str_res);
    }
    else
    {
        m_prereq->Show(false);
        m_properties_wnd->Show(false);
        m_cost->Show(false);
    }
    m_name->AdjustHeightToText();
    m_cost->AdjustHeightToText();
    m_desc->AdjustHeightToText();
    m_prereq->AdjustHeightToText();

    Fvector2 new_pos;
    new_pos.x = m_cost->GetWndPos().x;
    new_pos.y = m_name->GetWndPos().y + m_name->GetWndSize().y + 5.0f;
    m_cost->SetWndPos(new_pos);

    new_pos.x = m_desc->GetWndPos().x;
    new_pos.y = m_cost->GetWndPos().y + m_cost->GetWndSize().y + 5.0f;
    m_desc->SetWndPos(new_pos);

    new_pos.x = m_prereq->GetWndPos().x;
    new_pos.y = m_desc->GetWndPos().y + m_desc->GetWndSize().y + 5.0f;
    m_prereq->SetWndPos(new_pos);

    new_pos.x = m_properties_wnd->GetWndPos().x;
    new_pos.y = m_prereq->GetWndPos().y + m_prereq->GetWndSize().y + 5.0f;
    m_properties_wnd->SetWndPos(new_pos);

    m_properties_wnd->set_upgrade_info(*m_upgrade);

    // this wnd
    Fvector2 new_size;
    new_size.x = GetWndSize().x;
    new_size.y = m_properties_wnd->GetWndPos().y + m_properties_wnd->GetWndSize().y + 10.0f;
    SetWndSize(new_size);
    m_background->SetWndSize(new_size);

    return true;
}

void UIInvUpgradeInfo::Draw()
{
    if (m_upgrade)
    {
        inherited::Draw();
    }
}
