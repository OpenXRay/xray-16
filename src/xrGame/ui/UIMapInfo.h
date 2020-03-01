#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUIScrollView;

class CUIMapInfo : public CUIWindow
{
public:
    CUIMapInfo();
    ~CUIMapInfo();
    void InitMapInfo(Fvector2 pos, Fvector2 size);
    void InitMap(LPCSTR map_name, LPCSTR map_ver);
    LPCSTR GetLargeDesc();

protected:
    CUIScrollView* m_view;
    shared_str m_large_desc;
};
