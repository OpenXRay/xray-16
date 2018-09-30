#include "StdAfx.h"
#include "UIKeyBinding.h"
#include "UIXmlInit.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIEditKeyBind.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xr_level_controller.h"
#include "string_table.h"

CUIKeyBinding::CUIKeyBinding()
{
    for (int i = 0; i < 3; i++)
        AttachChild(&m_header[i]);

    AttachChild(&m_frame);
}

void CUIKeyBinding::InitFromXml(CUIXml& xml_doc, LPCSTR path)
{
    CUIXmlInit::InitWindow(xml_doc, path, 0, this);
    string256 buf;
    m_scroll_wnd = new CUIScrollView();
    m_scroll_wnd->SetAutoDelete(true);
    AttachChild(m_scroll_wnd);
    CUIXmlInit::InitScrollView(xml_doc, strconcat(sizeof(buf), buf, path, ":scroll_view"), 0, m_scroll_wnd);

    CUIXmlInit::InitFrameWindow(xml_doc, strconcat(sizeof(buf), buf, path, ":frame"), 0, &m_frame);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_1"), 0, &m_header[0]);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_2"), 0, &m_header[1]);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_3"), 0, &m_header[2]);

    FillUpList(xml_doc, path);
}

void CUIKeyBinding::FillUpList(CUIXml& xml_doc_ui, LPCSTR path_ui)
{
    string256 buf;
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_keybinding.xml");

    int groupsCount = xml_doc.GetNodesNum("", 0, "group");

    for (int i = 0; i < groupsCount; ++i)
    {
        // add group
        shared_str grp_name = xml_doc.ReadAttrib("group", i, "name");
        R_ASSERT(xr_strlen(grp_name));

        CUIStatic* pItem = new CUIStatic();
        CUIXmlInit::InitStatic(xml_doc_ui, strconcat(sizeof(buf), buf, path_ui, ":scroll_view:item_group"), 0, pItem);
        pItem->TextItemControl()->SetTextST(grp_name.c_str());
        m_scroll_wnd->AddWindow(pItem, true);

        // add group items
        int commandsCount = xml_doc.GetNodesNum("group", i, "command");
        XML_NODE tab_node = xml_doc.NavigateToNode("group", i);
        xml_doc.SetLocalRoot(tab_node);

        for (int j = 0; j < commandsCount; ++j)
        {
            // first field of list item
            shared_str command_id = xml_doc.ReadAttrib("command", j, "id");

            pItem = new CUIStatic();
            CUIXmlInit::InitStatic(xml_doc_ui, strconcat(sizeof(buf), buf, path_ui, ":scroll_view:item_key"), 0, pItem);
            pItem->TextItemControl()->SetTextST(command_id.c_str());
            m_scroll_wnd->AddWindow(pItem, true);

            shared_str exe = xml_doc.ReadAttrib("command", j, "exe");

#ifdef DEBUG
            if (kNOTBINDED == action_name_to_id(*exe))
            {
                Msg("action [%s] not exist. update data", exe.c_str());
                continue;
            }
#endif

            float item_width = m_header[1].GetWidth() - 3.0f;
            float item_pos = m_header[1].GetWndPos().x;
            CUIEditKeyBind* pEditKB = new CUIEditKeyBind(true);
            pEditKB->SetAutoDelete(true);
            pEditKB->InitKeyBind(Fvector2().set(item_pos, 0.0f), Fvector2().set(item_width, pItem->GetWndSize().y));
            pEditKB->AssignProps(exe, "key_binding");
            pItem->AttachChild(pEditKB);

            item_width = m_header[2].GetWidth() - 3.0f;
            item_pos = m_header[2].GetWndPos().x;
            pEditKB = new CUIEditKeyBind(false);
            pEditKB->SetAutoDelete(true);
            pEditKB->InitKeyBind(Fvector2().set(item_pos, 0.0f), Fvector2().set(item_width, pItem->GetWndSize().y));
            pEditKB->AssignProps(exe, "key_binding");
            pItem->AttachChild(pEditKB);
        }
        xml_doc.SetLocalRoot(xml_doc.GetRoot());
    }
#ifdef DEBUG
    CheckStructure(xml_doc);
#endif
}

#ifdef DEBUG
void CUIKeyBinding::CheckStructure(CUIXml& xml_doc)
{
    bool first = true;
    CUITextWnd* pItem = nullptr;

    for (int i = 0; true; i++)
    {
        LPCSTR action_name = actions[i].action_name;
        if (action_name)
        {
            if (IsActionExist(action_name, xml_doc))
                continue;
            else
            {
                if (first)
                {
                    pItem = new CUITextWnd();
                    pItem->SetWndPos(Fvector2().set(0, 0));
                    pItem->SetWndSize(Fvector2().set(m_scroll_wnd->GetWndSize().x, 20.0f));
                    pItem->SetText("NEXT ITEMS NOT DESCRIBED IN COMMAND DESC LIST");
                    first = false;
                    pItem->SetAutoDelete(true);
                    m_scroll_wnd->AddWindow(pItem, true);
                }

                pItem = new CUITextWnd();
                pItem->SetWndPos(Fvector2().set(0, 0));
                pItem->SetWndSize(Fvector2().set(m_scroll_wnd->GetWndSize().x, 20.0f));
                pItem->SetText(action_name);
                pItem->SetAutoDelete(true);
                m_scroll_wnd->AddWindow(pItem, true);
            }
        }
        else
            break;
    }
}

bool CUIKeyBinding::IsActionExist(LPCSTR action, CUIXml& xml_doc)
{
    bool ret = false;
    int groupsCount = xml_doc.GetNodesNum("", 0, "group");

    for (int i = 0; i < groupsCount; i++)
    {
        // add group items
        int commandsCount = xml_doc.GetNodesNum("group", i, "command");
        XML_NODE tab_node = xml_doc.NavigateToNode("group", i);
        xml_doc.SetLocalRoot(tab_node);

        for (int j = 0; j < commandsCount; ++j)
        {
            // first field of list item
            shared_str command_id = xml_doc.ReadAttrib("command", j, "exe");
            if (0 == xr_strcmp(action, *command_id))
            {
                ret = true;
                break;
            }
        }
        xml_doc.SetLocalRoot(xml_doc.GetRoot());
        if (ret)
            break;
    }
    return ret;
}
#endif
