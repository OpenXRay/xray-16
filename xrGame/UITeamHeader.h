#ifndef UI_TEAM_HEADER
#define UI_TEAM_HEADER

#include "ui/UIWindow.h"
#include "ui/xrUIXmlParser.h"
#include "ui/UIXmlInit.h"

#include "game_cl_base.h"
#include "string_table.h"
#include "level.h"
#include "associative_vector.h"

class CUIStatic;

class UITeamState;

class UITeamHeader : public CUIWindow
{
private:
	typedef	CUIWindow									inherited;
	
	typedef associative_vector<shared_str, CUIStatic*>		FieldsStatics;
	typedef associative_vector<shared_str, CUIStatic*>		ColumnsStatics;
	typedef associative_vector<shared_str, STRING_VALUE>	TranslatedStrings;
	
	FieldsStatics				m_field_fillers;
	ColumnsStatics				m_columns_statics;
	TranslatedStrings			m_translated_strings;

	UITeamState const * const	m_parent;
	XML_NODE*					m_team_header_root;

	void			InitColumnsStatics	(CUIXml& uiXml);
	void			InitFieldsStatics	(CUIXml& uiXml);
public:
					UITeamHeader	(UITeamState const * const parent);
	virtual			~UITeamHeader	();
			void	Init			(CUIXml& uiXml, LPCSTR path);
	virtual	void	Update			();

protected:
private:
}; //UITeamHeader


#endif //UI_TEAM_HEADER