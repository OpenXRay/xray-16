////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInventoryUpgradeWnd_add.cpp
//	Created 	: 08.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI window (additional) class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "Common/object_broker.h"
#include "UIInventoryUpgradeWnd.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "string_table.h"

void CUIInventoryUpgradeWnd::LoadCellsBacks(CUIXml& uiXml)
{
    XML_NODE stored_root = uiXml.GetLocalRoot();

    int cnt = uiXml.GetNodesNum("cell_states", 0, "state");

    XML_NODE node = uiXml.NavigateToNode("cell_states", 0);
    uiXml.SetLocalRoot(node);
    for (int i_st = 0; i_st < cnt; ++i_st)
    {
        uiXml.SetLocalRoot(uiXml.NavigateToNode("state", i_st));

        LPCSTR type = uiXml.Read("type", 0, "");
        LPCSTR txr = uiXml.Read("back_texture", 0, NULL);
        LPCSTR txr2 = uiXml.Read("point_texture", 0, NULL);
        u32 color = CUIXmlInit::GetColor(uiXml, "item_color", 0, 0);
        LoadCellStates(type, txr, txr2, color);

        uiXml.SetLocalRoot(node);
    }
    uiXml.SetLocalRoot(stored_root);
}

void CUIInventoryUpgradeWnd::LoadCellStates(LPCSTR state_str, LPCSTR texture_name, LPCSTR texture_name2, u32 color)
{
    VERIFY(state_str && xr_strcmp(state_str, ""));
    if (texture_name && !xr_strcmp(texture_name, ""))
    {
        texture_name = NULL;
    }
    if (texture_name2 && !xr_strcmp(texture_name2, ""))
    {
        texture_name2 = NULL;
    }

    SetCellState(SelectCellState(state_str), texture_name, texture_name2, color);
}

UIUpgrade::ViewState CUIInventoryUpgradeWnd::SelectCellState(LPCSTR state_str)
{
    if (!xr_strcmp(state_str, "enabled"))
    {
        return UIUpgrade::STATE_ENABLED;
    }
    if (!xr_strcmp(state_str, "highlight"))
    {
        return UIUpgrade::STATE_FOCUSED;
    }
    if (!xr_strcmp(state_str, "touched"))
    {
        return UIUpgrade::STATE_TOUCHED;
    }
    if (!xr_strcmp(state_str, "selected"))
    {
        return UIUpgrade::STATE_SELECTED;
    }
    if (!xr_strcmp(state_str, "unknown"))
    {
        return UIUpgrade::STATE_UNKNOWN;
    }

    if (!xr_strcmp(state_str, "disabled_parent"))
    {
        return UIUpgrade::STATE_DISABLED_PARENT;
    }
    if (!xr_strcmp(state_str, "disabled_group"))
    {
        return UIUpgrade::STATE_DISABLED_GROUP;
    }
    if (!xr_strcmp(state_str, "disabled_money"))
    {
        return UIUpgrade::STATE_DISABLED_PREC_MONEY;
    }
    if (!xr_strcmp(state_str, "disabled_quest"))
    {
        return UIUpgrade::STATE_DISABLED_PREC_QUEST;
    }

    if (!xr_strcmp(state_str, "disabled_highlight"))
    {
        return UIUpgrade::STATE_DISABLED_FOCUSED;
    }

    VERIFY2(0, make_string("Such UI upgrade state (%s) does not exist !", state_str));
    return UIUpgrade::STATE_UNKNOWN;
}

void CUIInventoryUpgradeWnd::SetCellState(
    UIUpgrade::ViewState state, LPCSTR texture_name, LPCSTR texture_name2, u32 color)
{
    m_cell_textures[state] = texture_name;
    m_point_textures[state] = texture_name2;
}

bool CUIInventoryUpgradeWnd::VerirfyCells()
{
    for (int i = 0; i < UIUpgrade::STATE_COUNT; ++i)
    {
        if (!m_cell_textures[i]._get())
            return false;
    }
    return true;
}

void CUIInventoryUpgradeWnd::LoadSchemes(CUIXml& uiXml)
{
    XML_NODE stored_root = uiXml.GetLocalRoot();

    XML_NODE tmpl_root = uiXml.NavigateToNode("templates", 0);
    uiXml.SetLocalRoot(tmpl_root);

    Frect t_cell_item;

    t_cell_item.x1 = uiXml.ReadAttribFlt("cell_item", 0, "x");
    t_cell_item.y1 = uiXml.ReadAttribFlt("cell_item", 0, "y");
    t_cell_item.x2 =
        t_cell_item.x1 + uiXml.ReadAttribFlt("cell_item", 0, "width") * (UI().is_widescreen() ? 0.8f : 1.0f);
    t_cell_item.y2 = t_cell_item.y1 + uiXml.ReadAttribFlt("cell_item", 0, "height");

    int tmpl_count = uiXml.GetNodesNum(tmpl_root, "template");
    for (int i_tmpl = 0; i_tmpl < tmpl_count; ++i_tmpl)
    {
        XML_NODE tmpl_node = uiXml.NavigateToNode("template", i_tmpl);
        uiXml.SetLocalRoot(tmpl_node);

        Scheme* scheme = new Scheme();
        scheme->cells.reserve(MAX_UI_UPGRADE_CELLS);

        LPCSTR name = uiXml.ReadAttrib(tmpl_node, "name", "");
        VERIFY(name && xr_strcmp(name, ""));
        scheme->name._set(name);

        int clm_count = uiXml.GetNodesNum(tmpl_node, "column");
        for (int i_clm = 0; i_clm < clm_count; ++i_clm)
        {
            XML_NODE clm_node = uiXml.NavigateToNode("column", i_clm);
            uiXml.SetLocalRoot(clm_node);

            int cell_cnt = uiXml.GetNodesNum(clm_node, "cell");
            for (int i_cell = 0; i_cell < cell_cnt; ++i_cell)
            {
                UIUpgrade* item = new UIUpgrade(this);
                item->load_from_xml(uiXml, i_clm, i_cell, t_cell_item);
                CUIUpgradePoint* item_point = new CUIUpgradePoint(item);
                item_point->load_from_xml(uiXml, i_cell);
                item->attach_point(item_point);

                scheme->cells.push_back(item);
            } // for i_cell

            uiXml.SetLocalRoot(tmpl_node);
        } // for i_clm

        m_schemes.push_back(scheme);
        uiXml.SetLocalRoot(tmpl_root);
    } // for i_tmpl

    uiXml.SetLocalRoot(stored_root);
}
