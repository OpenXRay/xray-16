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
    typedef struct
    {
        ui_shader sh;
        Frect rect;
    } TEX_INFO;

    static void InitTexInfo();
    static void FreeTexInfo();

    using tex_info_data = std::array<TEX_INFO[2], MAX_DEF_TEX>;

    inline static tex_info_data* m_tex_info;
};
