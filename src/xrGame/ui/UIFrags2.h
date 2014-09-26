#pragma once

#include "UIFrags.h"

class CUIFrags2 : public CUIFrags{
public:
	CUIFrags2();
	~CUIFrags2();
	void Init(CUIXml& xml_doc, LPCSTR path, LPCSTR backgrnd_path);
protected:
	CUIStats*	m_pStats2;
};