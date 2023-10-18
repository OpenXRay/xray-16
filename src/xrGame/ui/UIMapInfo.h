#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUIScrollView;

class CUIMapInfo final : public CUIWindow
{
public:
    CUIMapInfo();
    ~CUIMapInfo() override;
    void InitMapInfo(Fvector2 pos, Fvector2 size);
    void InitMap(LPCSTR map_name, LPCSTR map_ver);
    LPCSTR GetLargeDesc();
    pcstr GetDebugType() override { return "CUIMapInfo"; }

protected:
    CUIScrollView* m_view;
    shared_str m_large_desc;
};
