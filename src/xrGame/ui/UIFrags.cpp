#include "StdAfx.h"

#include "UIFrags.h"
#include "UIStats.h"
#include "UIXmlInit.h"
#include "UIStatic.h"

CUIFrags::CUIFrags(){	
	m_pBackT = xr_new<CUIStatic>(); AttachChild(m_pBackT);
	m_pBackC = xr_new<CUIStatic>(); AttachChild(m_pBackC);
	m_pBackB = xr_new<CUIStatic>(); AttachChild(m_pBackB);
	m_pStats = xr_new<CUIStats>();  AttachChild(m_pStats);
}

CUIFrags::~CUIFrags(){
	xr_delete(m_pStats);
	xr_delete(m_pBackT);
	xr_delete(m_pBackC);
	xr_delete(m_pBackB);

}

void CUIFrags::Init(CUIXml& xml_doc, LPCSTR path, LPCSTR backgrnd_path)
{
	m_pStats->InitStats	(xml_doc, path, 0);
	InitBackground		(xml_doc, backgrnd_path);	
}

void CUIFrags::InitBackground(CUIXml& xml_doc, LPCSTR path){
	string256 _path;
	CUIXmlInit::InitWindow(xml_doc, path, 0, this);
	CUIXmlInit::InitStatic(xml_doc, strconcat(sizeof(_path),_path, path, ":back_c"), 0, m_pBackC);
	int count = xml_doc.ReadAttribInt(_path, 0, "count", 1);
	m_pBackC->GetStaticItem()->SetTile(1,count,0,0);
	CUIXmlInit::InitStatic(xml_doc, strconcat(sizeof(_path),_path, path, ":back_t"), 0, m_pBackT);
	CUIXmlInit::InitStatic(xml_doc, strconcat(sizeof(_path),_path, path, ":back_b"), 0, m_pBackB);

}