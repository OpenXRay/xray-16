#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIStatsIcon : public CUIStatic
{
    friend class CUIStatsPlayerList;
    friend class UITeamPanels;

public:
    CUIStatsIcon();
    void SetValue(LPCSTR str);
#ifdef XR_PLATFORM_SWITCH
    // Is used to init static/global members 
    static void GlobalInit();

    typedef struct
    {
        ui_shader sh;
        Frect rect;
    } TEX_INFO2;
#endif

protected:
    enum DEF_TEX
    {
        RANK_0 = 0,
        RANK_1,
        RANK_2,
        RANK_3,
        RANK_4,
        RANK_5,
        ARTEFACT,
        DEATH,

        MAX_DEF_TEX
    };

    static void InitTexInfo();
    static void FreeTexInfo();

#ifndef XR_PLATFORM_SWITCH
    typedef struct
    {
        ui_shader sh;
        Frect rect;
    } TEX_INFO2;
    
    static TEX_INFO2 m_tex_info[MAX_DEF_TEX][2];
#else
    static TEX_INFO2** m_tex_info;
#endif
};
