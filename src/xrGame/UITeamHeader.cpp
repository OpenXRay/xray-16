#include "StdAfx.h"
#include "UITeamHeader.h"
#include "UITeamState.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrCore/buffer_vector.h"

UITeamHeader::UITeamHeader(UITeamState const* const parent) : m_parent(parent), m_team_header_root() {}
UITeamHeader::~UITeamHeader() {}
void UITeamHeader::Update()
{
    inherited::Update();
    VERIFY(m_parent);
    FieldsStatics::iterator ie = m_field_fillers.end();
    for (FieldsStatics::iterator i = m_field_fillers.begin(); i != ie; ++i)
    {
        s32 temp_value = m_parent->GetFieldValue(i->first);
        CUIStatic* fieldStatic = i->second;
        VERIFY2(fieldStatic, make_string("field %s not initialized", i->first.c_str()).c_str());
        STRING_VALUE temp_text = m_translated_strings[i->first];
        buffer_vector<char>::size_type new_size = temp_text.size() + 16; // i hope STRING_VALUE has size() method :)
        buffer_vector<char> new_string(_alloca(new_size), new_size);
        xr_sprintf(new_string.begin(), new_size, "%s: %d", temp_text.c_str(), temp_value);
        fieldStatic->TextItemControl()->SetText(new_string.begin());
    }
}

#define COLUMN_NODE_NAME "column"
void UITeamHeader::InitColumnsStatics(CUIXml& uiXml)
{
    VERIFY(m_team_header_root);
    int tempNumber = uiXml.GetNodesNum(m_team_header_root, COLUMN_NODE_NAME);
    for (int i = 0; i < tempNumber; ++i)
    {
        XML_NODE tempColumnNode = uiXml.NavigateToNode(COLUMN_NODE_NAME, i);
        if (!tempColumnNode)
            break;
        LPCSTR tempColumnName = uiXml.ReadAttrib(tempColumnNode, "name", "column_not_set_in_name_attribute");
        CUIStatic* tempColumn = new CUIStatic();
        VERIFY(tempColumn);
        this->AttachChild(static_cast<CUIWindow*>(tempColumn));
        tempColumn->SetAutoDelete(true);
        CUIXmlInit::InitStatic(uiXml, COLUMN_NODE_NAME, i, tempColumn);
        // tempColumn->SetTextST(tempColumnName);
        m_columns_statics.insert(std::make_pair(shared_str(tempColumnName), tempColumn));
    }
}

#define FILED_NODE_NAME "field"
void UITeamHeader::InitFieldsStatics(CUIXml& uiXml)
{
    VERIFY(m_team_header_root);
    int tempNumber = uiXml.GetNodesNum(m_team_header_root, FILED_NODE_NAME);
    for (int i = 0; i < tempNumber; ++i)
    {
        XML_NODE tempFieldNode = uiXml.NavigateToNode(FILED_NODE_NAME, i);
        if (!tempFieldNode)
            break;
        LPCSTR tempFieldName = uiXml.ReadAttrib(tempFieldNode, "name", "field_not_set_in_name_attribute");
        CUIStatic* tempField = new CUIStatic();
        VERIFY(tempField);
        this->AttachChild(static_cast<CUIWindow*>(tempField));
        tempField->SetAutoDelete(true);
        CUIXmlInit::InitStatic(uiXml, FILED_NODE_NAME, i, tempField);
        m_translated_strings.insert(std::make_pair(shared_str(tempFieldName), StringTable().translate(tempFieldName)));
        m_field_fillers.insert(std::make_pair(shared_str(tempFieldName), tempField));
    }
}

void UITeamHeader::Init(CUIXml& uiXml, LPCSTR path)
{
    CUIXmlInit::InitWindow(uiXml, path, 0, this);
    m_team_header_root = uiXml.NavigateToNode(path, 0);
    VERIFY(m_team_header_root);
    XML_NODE prevRoot = uiXml.GetLocalRoot();
    VERIFY(prevRoot);
    uiXml.SetLocalRoot(m_team_header_root);
    InitColumnsStatics(uiXml);
    InitFieldsStatics(uiXml);
    uiXml.SetLocalRoot(prevRoot);
}
