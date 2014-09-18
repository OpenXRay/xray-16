#pragma once

#include "UIScrollView.h"
#include "UIStatsPlayerList.h"

class CUIXml;
class CUIFrameWindow;

class CUIStats : public CUIScrollView {
public:
				CUIStats		();
	virtual		~CUIStats		();
	CUIWindow* InitStats		(CUIXml& xml_doc, LPCSTR path,int team);
};