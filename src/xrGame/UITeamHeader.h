#ifndef UI_TEAM_HEADER
#define UI_TEAM_HEADER

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "ui/UIXmlInit.h"

#include "game_cl_base.h"
#include "string_table.h"
#include "Level.h"
#include "xrCore/Containers/AssociativeVector.hpp"

class CUIStatic;

class UITeamState;

class UITeamHeader : public CUIWindow
{
private:
    typedef CUIWindow inherited;

    typedef AssociativeVector<shared_str, CUIStatic*> FieldsStatics;
    typedef AssociativeVector<shared_str, CUIStatic*> ColumnsStatics;
    typedef AssociativeVector<shared_str, STRING_VALUE> TranslatedStrings;

    FieldsStatics m_field_fillers;
    ColumnsStatics m_columns_statics;
    TranslatedStrings m_translated_strings;

    UITeamState const* const m_parent;
    XML_NODE m_team_header_root;

    void InitColumnsStatics(CUIXml& uiXml);
    void InitFieldsStatics(CUIXml& uiXml);

public:
    UITeamHeader(UITeamState const* const parent);
    virtual ~UITeamHeader();
    void Init(CUIXml& uiXml, LPCSTR path);
    virtual void Update();

protected:
private:
}; // UITeamHeader

#endif // UI_TEAM_HEADER
