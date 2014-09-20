#include "stdafx.h"
#include "xrUIXmlParser.h"

#ifdef XRGAME_EXPORTS
#	include "../ui_base.h"
#endif // XRGAME_EXPORTS

shared_str CUIXml::correct_file_name	(LPCSTR path, LPCSTR fn)
{
#ifdef XRGAME_EXPORTS
	if(0==xr_strcmp(path,UI_PATH) || 0==xr_strcmp(path,"UI"))
	{
		return UI()->get_xml_name(fn);
	}else
		return fn;
#else
	return fn;
#endif
}

//#define LOG_ALL_XMLS
#ifdef LOG_ALL_XMLS
int ListXmlCount = 0;
struct DBGList_{
	int				num;
	bool			closed;
};
xr_vector<DBGList_>	dbg_list_xmls;
void dump_list_xmls()
{
	Msg("------Total  xmls %d",dbg_list_xmls.size());
	xr_vector<DBGList_>::iterator _it = dbg_list_xmls.begin();
	for(;_it!=dbg_list_xmls.end();++_it)
		if(!(*_it).closed)
			Msg("--leak detected ---- xml = %d",(*_it).num);
}
#else
	void dump_list_xmls(){}
#endif

CUIXml::CUIXml()
{
#ifdef LOG_ALL_XMLS
	ListXmlCount++;
	m_dbg_id = ListXmlCount;
	dbg_list_xmls.push_back(DBGList_());
	dbg_list_xmls.back().num		= m_dbg_id;
	dbg_list_xmls.back().closed		= false;
#endif
}

CUIXml::~CUIXml()
{
#ifdef LOG_ALL_XMLS
	xr_vector<DBGList_>::iterator _it = dbg_list_xmls.begin();
	bool bOK = false;
	for(;_it!=dbg_list_xmls.end();++_it){
		if( (*_it).num==m_dbg_id && !(*_it).closed){
			bOK = true;
			(*_it).closed = true;
			dbg_list_xmls.erase(_it);
			break;
		}
		if( (*_it).num==m_dbg_id && (*_it).closed){
			Msg("--XML [%d] already deleted", m_dbg_id);
			bOK = true;
		}
	}
	if(!bOK)
		Msg("CUIXml::~CUIXml.[%d] cannot find xml in list", m_dbg_id);
#endif
}
