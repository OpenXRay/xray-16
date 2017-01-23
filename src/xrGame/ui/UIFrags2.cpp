#include "StdAfx.h"

#include "UIFrags2.h"
#include "UIStats.h"
#include "UIXmlInit.h"
#include "UIStatic.h"

CUIFrags2::CUIFrags2(){	
	m_pStats2 = xr_new<CUIStats>();  AttachChild(m_pStats2);
}

CUIFrags2::~CUIFrags2(){
	xr_delete(m_pStats2);
}


void CUIFrags2::Init(CUIXml& xml_doc, LPCSTR path, LPCSTR backgrnd_path){
	InitBackground(xml_doc, backgrnd_path);

	CUIWindow* pTeam1 = NULL;
	CUIWindow* pTeam2 = NULL;
	Fvector2 pos;

	pTeam1 = m_pStats->InitStats(xml_doc, path, 1);
	AttachChild(pTeam1);
	pTeam2 = m_pStats2->InitStats(xml_doc, path, 2);
	AttachChild(pTeam2);

    // team 2 list
	float x = xml_doc.ReadAttribFlt(path, 0, "x2");
	R_ASSERT(x);
	pos = m_pStats2->GetWndPos();
	pos.x = x;				// 
	pos.y += 3;	
	m_pStats2->SetWndPos(pos);
	// team2 statas
	pos = pTeam2->GetWndPos();
	pos.x += m_pStats2->GetWndPos().x;
	pTeam2->SetWndPos(pos);

    // team 1 list
	pos = m_pStats->GetWndPos();
	pos.y += 3;
	m_pStats->SetWndPos(pos);

	pos = pTeam1->GetWndPos();
	pos.x += m_pStats->GetWndPos().x;
	pTeam1->SetWndPos(pos);
	

}