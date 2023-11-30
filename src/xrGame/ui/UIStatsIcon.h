#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIStatsIcon final : public CUIStatic
{
    friend class CUIStatsPlayerList;
    friend class UITeamPanels;

public:
    CUIStatsIcon();
    void SetValue(LPCSTR str);
    pcstr GetDebugType() override { return "CUIStatsIcon"; }

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

protected:
    static void InitTexInfo();
    static void FreeTexInfo();
};
