#include "StdAfx.h"
#include "UIKeyBinding.h"
#include "UIXmlInit.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIEditKeyBind.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrEngine/xr_level_controller.h"

CUIKeyBinding::CUIKeyBinding()
    : CUIWindow(CUIKeyBinding::GetDebugType()),
      m_header{ "Game actions header", "Primary bindings header", "Secondary bindings header" },
      m_frame( "Frame" )
{
    for (auto& header : m_header)
        AttachChild(&header);

    AttachChild(&m_frame);

    pInput->RegisterKeyMapChangeWatcher(this, REG_PRIORITY_NORMAL);
}

CUIKeyBinding::~CUIKeyBinding()
{
    pInput->RemoveKeyMapChangeWatcher(this);
}

void CUIKeyBinding::InitFromXml(CUIXml& xml_doc, LPCSTR path)
{
    CUIXmlInit::InitWindow(xml_doc, path, 0, this);
    string256 buf;
    m_scroll_wnd = xr_new<CUIScrollView>();
    m_scroll_wnd->SetAutoDelete(true);
    AttachChild(m_scroll_wnd);
    CUIXmlInit::InitScrollView(xml_doc, strconcat(sizeof(buf), buf, path, ":scroll_view"), 0, m_scroll_wnd);

    m_isGamepadBinds = !!(xml_doc.ReadAttribInt(path, 0, "gamepad_bind", 0));
    CUIXmlInit::InitFrameWindow(xml_doc, strconcat(sizeof(buf), buf, path, ":frame"), 0, &m_frame);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_1"), 0, &m_header[0]);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_2"), 0, &m_header[1]);
    if (!m_isGamepadBinds)
        CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header_3"), 0, &m_header[2]);

    FillUpList(xml_doc, path);
}

void CUIKeyBinding::FillUpList(CUIXml& xml_doc_ui, LPCSTR path_ui)
{
    string256 buf;
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, m_isGamepadBinds ? "ui_keybinding_gamepad.xml" : "ui_keybinding.xml");

    int groupsCount = xml_doc.GetNodesNum("", 0, "group");

    for (int i = 0; i < groupsCount; ++i)
    {
        // add group
        shared_str grp_name = xml_doc.ReadAttrib("group", i, "name");
        R_ASSERT(xr_strlen(grp_name));

        CUIStatic* item = xr_new<CUIStatic>(grp_name.c_str());
        CUIXmlInit::InitStatic(xml_doc_ui, strconcat(sizeof(buf), buf, path_ui, ":scroll_view:item_group"), 0, item);
        item->TextItemControl()->SetTextST(grp_name.c_str());
        m_scroll_wnd->AddWindow(item, true);

        // add group items
        int commandsCount = xml_doc.GetNodesNum("group", i, "command");
        XML_NODE tab_node = xml_doc.NavigateToNode("group", i);
        xml_doc.SetLocalRoot(tab_node);

        for (int j = 0; j < commandsCount; ++j)
        {
            // first field of list item
            shared_str command_id = xml_doc.ReadAttrib("command", j, "id");

            item = xr_new<CUIStatic>(command_id.c_str());
            CUIXmlInit::InitStatic(xml_doc_ui, strconcat(sizeof(buf), buf, path_ui, ":scroll_view:item_key"), 0, item);
            item->TextItemControl()->SetTextST(command_id.c_str());
            m_scroll_wnd->AddWindow(item, true);

            shared_str exe = xml_doc.ReadAttrib("command", j, "exe");

#ifdef DEBUG
            if (kNOTBINDED == ActionNameToId(*exe))
            {
                Msg("action [%s] not exist. update data", exe.c_str());
                continue;
            }
#endif

            float item_width = m_header[1].GetWidth() - 3.0f;
            float item_pos = m_header[1].GetWndPos().x;
            CUIEditKeyBind* editKB;
            if (!m_isGamepadBinds)
                editKB = xr_new<CUIEditKeyBind>(true);
            else
                editKB = xr_new<CUIEditKeyBind>(false, true);
            editKB->SetAutoDelete(true);
            editKB->InitKeyBind(Fvector2().set(item_pos, 0.0f), Fvector2().set(item_width, item->GetWndSize().y));
            if (!m_isGamepadBinds)
                editKB->AssignProps(exe, "key_binding");
            else
                editKB->AssignProps(exe, "key_binding_gamepad");
            item->AttachChild(editKB);

            if (!m_isGamepadBinds)
            {
                item_width = m_header[2].GetWidth() - 3.0f;
                item_pos = m_header[2].GetWndPos().x;
                editKB = xr_new<CUIEditKeyBind>(false);
                editKB->SetAutoDelete(true);
                editKB->InitKeyBind(Fvector2().set(item_pos, 0.0f), Fvector2().set(item_width, item->GetWndSize().y));
                editKB->AssignProps(exe, "key_binding");
                item->AttachChild(editKB);
            }
        }
        xml_doc.SetLocalRoot(xml_doc.GetRoot());
    }
#ifdef DEBUG
    CheckStructure(xml_doc);
#endif
}

void CUIKeyBinding::OnKeyMapChanged()
{
    for (CUIWindow* item : m_scroll_wnd->Items())
    {
        for (CUIWindow* wnd : item->GetChildWndList())
        {
            CUIEditKeyBind* keybind = smart_cast<CUIEditKeyBind*>(wnd);
            if (!keybind)
                continue;

            keybind->SetValue(); // re-apply
        }
    }
}

#ifdef DEBUG
void CUIKeyBinding::CheckStructure(CUIXml& xml_doc)
{
    bool first = true;
    CUITextWnd* item = nullptr;

    for (int i = 0; true; ++i)
    {
        LPCSTR action_name = actions[i].action_name;
        if (action_name)
        {
            if (!IsActionExist(action_name, xml_doc))
            {
                if (first)
                {
                    item = xr_new<CUITextWnd>();
                    item->SetWndPos(Fvector2().set(0, 0));
                    item->SetWndSize(Fvector2().set(m_scroll_wnd->GetWndSize().x, 20.0f));
                    item->SetText("NEXT ITEMS NOT DESCRIBED IN COMMAND DESC LIST");
                    first = false;
                    item->SetAutoDelete(true);
                    m_scroll_wnd->AddWindow(item, true);
                }

                item = xr_new<CUITextWnd>();
                item->SetWndPos(Fvector2().set(0, 0));
                item->SetWndSize(Fvector2().set(m_scroll_wnd->GetWndSize().x, 20.0f));
                item->SetText(action_name);
                item->SetAutoDelete(true);
                m_scroll_wnd->AddWindow(item, true);
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

    for (int i = 0; i < groupsCount; ++i)
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
