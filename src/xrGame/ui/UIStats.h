#pragma once

#include "xrUICore/ScrollView/UIScrollView.h"
#include "UIStatsPlayerList.h"

class CUIXml;
class CUIFrameWindow;

class CUIStats final : public CUIScrollView
{
public:
    CUIStats();
    CUIWindow* InitStats(CUIXml& xml_doc, LPCSTR path, int team);
    pcstr GetDebugType() override { return "CUIStats"; }
};
