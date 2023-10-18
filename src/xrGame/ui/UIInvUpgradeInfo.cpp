////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInvUpgradeInfo.cpp
//	Created 	: 21.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI info window class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIInvUpgradeInfo.h"
#include "Actor.h"

#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"
#include "xrUICore/Windows/UIFrameWindow.h"

#include "UIInvUpgradeProperty.h"

#include "inventory_upgrade.h"
#include "inventory_upgrade_property.h"
#include "UIHelper.h"

UIInvUpgradeInfo::UIInvUpgradeInfo() : CUIWindow(UIInvUpgradeInfo::GetDebugType()) {}

void UIInvUpgradeInfo::init_from_xml(LPCSTR xml_name)
{
    CUIXml ui_xml;
    ui_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name);

    XML_NODE stored_root = ui_xml.GetLocalRoot();
    XML_NODE node = ui_xml.NavigateToNode("upgrade_info", 0);
    ui_xml.SetLocalRoot(node);

    CUIXmlInit::InitWindow(ui_xml, "main_frame", 0, this);

    m_background = UIHelper::CreateFrameWindow(ui_xml, "background_frame", this);
    m_name = UIHelper::CreateStatic(ui_xml, "info_name", this);
    m_cost = UIHelper::CreateStatic(ui_xml, "info_cost", this, false);
    m_desc = UIHelper::CreateStatic(ui_xml, "info_desc", this);
    m_prereq = UIHelper::CreateStatic(ui_xml, "info_prerequisites", this);

    m_properties_wnd = xr_new<UIInvUpgPropertiesWnd>();
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
        if (m_cost)
        {
            luabind::functor<LPCSTR> cost_func;
            pcstr cost_func_str = "inventory_upgrades.get_upgrade_cost";
            R_ASSERT2(GEnv.ScriptEngine->functor(cost_func_str, cost_func), "Failed to get cost");
            m_cost->SetText(cost_func(m_upgrade->section()));
            m_cost->Show(true);
        }

        m_prereq->SetText(nullptr);
        if (!ClearSkyMode)
            m_prereq->SetTextColor(color_rgba(255, 90, 90, 255));

        string512 str_res{};
        auto set_result_string = [&](pcstr desc, bool add = false)
        {
            if (ClearSkyMode)
            {
                xr_strcpy(str_res, StringTable().translate(desc).c_str());
            }
            else if (add)
            {
                xr_sprintf(str_res, "%s\\n - %s", str_res,
                    StringTable().translate(desc).c_str());
            }
            else
            {
                xr_sprintf(str_res, "%s:\\n - %s",
                    StringTable().translate("st_upgr_disable").c_str(),
                    StringTable().translate(desc).c_str());
            }
        };

        const auto upg_res = m_upgrade->can_install(*inv_item, false);
        switch (upg_res)
        {
        case inventory::upgrade::result_e_installed:
            if (!ClearSkyMode)
                m_prereq->SetTextColor(color_rgba(117, 255, 123, 255));
            m_prereq->SetTextST("st_upgr_installed");
            break;

        case inventory::upgrade::result_e_unknown:
            set_result_string("st_upgr_unknown");
            if (m_cost)
                m_cost->Show(false);
            break;

        case inventory::upgrade::result_e_group:
            set_result_string("st_upgr_group");
            break;

        case inventory::upgrade::result_e_cant_do:
            set_result_string("st_upgr_cant_do");
            break;

        default:
        {
            switch (upg_res)
            {
            case inventory::upgrade::result_ok:
            case inventory::upgrade::result_e_precondition_money:
            case inventory::upgrade::result_e_precondition_quest:
                if (ClearSkyMode)
                {
                    m_prereq->SetText(m_upgrade->get_prerequisites());
                    break;
                }

                if (upg_res != inventory::upgrade::result_e_precondition_quest)
                    break;
                [[fallthrough]];

            default:
                strconcat(str_res, StringTable().translate("st_upgr_disable").c_str(), ":\\n", m_upgrade->get_prerequisites());

                if (upg_res == inventory::upgrade::result_e_parents)
                    set_result_string("st_upgr_parents", true);
            }
        }
        } // switch (upg_res)
        if (str_res[0])
            m_prereq->SetText(str_res);
    }
    else
    {
        m_prereq->Show(false);
        m_properties_wnd->Show(false);
        if (m_cost)
            m_cost->Show(false);
    }
    m_name->AdjustHeightToText();
    if (m_cost)
        m_cost->AdjustHeightToText();
    m_desc->AdjustHeightToText();
    m_prereq->AdjustHeightToText();

    Fvector2 new_pos;
    if (m_cost)
    {
        new_pos.x = m_cost->GetWndPos().x;
        new_pos.y = m_name->GetWndPos().y + m_name->GetWndSize().y + 5.0f;
        m_cost->SetWndPos(new_pos);

        new_pos.x = m_desc->GetWndPos().x;
        new_pos.y = m_cost->GetWndPos().y + m_cost->GetWndSize().y + 5.0f;
        m_desc->SetWndPos(new_pos);
    }
    else
    {
        new_pos.x = m_desc->GetWndPos().x;
        new_pos.y = m_name->GetWndPos().y + m_name->GetWndSize().y + 5.0f;
        m_desc->SetWndPos(new_pos);
    }

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
