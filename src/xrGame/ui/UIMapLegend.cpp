////////////////////////////////////////////////////////////////////////////
//	Module 		: UIMapLegend.cpp
//	Created 	: 03.06.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Map Legend Wnd (PDA : Task) class impl
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIMapLegend.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIHelper.h"

#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Hint/UIHint.h"

#include "UIInventoryUtilities.h"
#include "string_table.h"
#include "Level.h"

UIMapLegend::UIMapLegend() {}
UIMapLegend::~UIMapLegend() { xr_delete(m_list); }
void UIMapLegend::init_from_xml(CUIXml& xml, LPCSTR path)
{
    CUIXmlInit::InitWindow(xml, path, 0, this);

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE tmpl_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(tmpl_root);

    m_background = UIHelper::CreateFrameWindow(xml, "background_frame", this);
    m_caption = UIHelper::CreateStatic(xml, "t_caption", this);
    m_btn_close = UIHelper::Create3tButton(xml, "btn_close", this);

    m_list = new CUIScrollView();
    AttachChild(m_list);
    CUIXmlInit::InitScrollView(xml, "legend_list", 0, m_list);

    UIMapLegendItem* list_item = NULL;

    int cn = xml.GetNodesNum("legend_list", 0, "item");
    XML_NODE root2 = xml.NavigateToNode("legend_list", 0);
    xml.SetLocalRoot(root2);

    for (int i = 0; i < cn; ++i)
    {
        list_item = new UIMapLegendItem();
        list_item->init_from_xml(xml, i);
        m_list->AddWindow(list_item, true);

        xml.SetLocalRoot(root2);
    }
    xml.SetLocalRoot(stored_root);
}

void UIMapLegend::Show(bool status) { inherited::Show(status); }
void UIMapLegend::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_btn_close)
    {
        if (msg == BUTTON_DOWN)
        {
            Show(false);
            return;
        }
    }
    inherited::SendMessage(pWnd, msg, pData);
}

// =================================================================================================

UIMapLegendItem::UIMapLegendItem() {}
UIMapLegendItem::~UIMapLegendItem() {}
void UIMapLegendItem::init_from_xml(CUIXml& xml, int index)
{
    CUIXmlInit::InitWindow(xml, "item", index, this);

    XML_NODE root3 = xml.NavigateToNode("item", index);
    xml.SetLocalRoot(root3);

    m_image[0] = UIHelper::CreateStatic(xml, "image", this);

    if (xml.NavigateToNode("image_1"))
    {
        m_image[1] = UIHelper::CreateStatic(xml, "image_1", this);
    }
    if (xml.NavigateToNode("image_2"))
    {
        m_image[2] = UIHelper::CreateStatic(xml, "image_2", this);
    }
    if (xml.NavigateToNode("image_3"))
    {
        m_image[3] = UIHelper::CreateStatic(xml, "image_3", this);
    }

    m_text = UIHelper::CreateStatic(xml, "text_static", this);
    m_text->AdjustHeightToText();

    float h = m_text->GetWndPos().y + m_text->GetHeight();
    h = _max(h, GetHeight());
    SetHeight(h);
}
