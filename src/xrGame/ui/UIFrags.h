#pragma once

#include "UIWindow.h"

class CUIXml;
class CUIStats;
class CUIStatic;

class CUIFrags : public CUIWindow{
public:
	CUIFrags();
	~CUIFrags();
	void Init(CUIXml& xml_doc, LPCSTR path, LPCSTR backgrnd_path);

protected:
	void InitBackground(CUIXml& xml_doc, LPCSTR path);
	CUIStatic*	m_pBackT;
	CUIStatic*	m_pBackC;
	CUIStatic*	m_pBackB;
	CUIStats*	m_pStats;
};