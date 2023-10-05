#include "StdAfx.h"
#include "UIStatsIcon.h"
#include "xrUICore/XML/UITextureMaster.h"
#include "UIInventoryUtilities.h"
#include <optional>

#include "Include/xrRender/UIShader.h"

struct CUIStatsIconTextureInfo
{
    ui_shader sh;
    Frect rect;
};

struct CUIStatsIconTextures {
    CUIStatsIconTextureInfo data[CUIStatsIcon::MAX_DEF_TEX][2];
};

std::optional<CUIStatsIconTextures> gTexInfo;

CUIStatsIcon::CUIStatsIcon()
    : CUIStatic("CUIStatsIcon")
{
    SetStretchTexture(true);
    InitTexInfo();
}

void CUIStatsIcon::InitTexInfo()
{
    if (!gTexInfo.has_value())
        gTexInfo.emplace();

    if (gTexInfo.value().data[RANK_0][0].sh->inited())
        return;
    // ranks
    string128 rank_tex;
    for (int i = RANK_0; i <= RANK_4; i++)
    {
        xr_sprintf(rank_tex, "ui_hud_status_green_0%d", i + 1);
        CUITextureMaster::GetTextureShader(rank_tex, gTexInfo.value().data[i][0].sh);
        gTexInfo.value().data[i][0].rect = CUITextureMaster::GetTextureRect(rank_tex);

        xr_sprintf(rank_tex, "ui_hud_status_blue_0%d", i + 1);
        CUITextureMaster::GetTextureShader(rank_tex, gTexInfo.value().data[i][1].sh);
        gTexInfo.value().data[i][1].rect = CUITextureMaster::GetTextureRect(rank_tex);
    }

    // artefact
    LPCSTR artefact_name = pSettings->r_string("artefacthunt_gamedata", "artefact");
    float fGridWidth = pSettings->r_float(artefact_name, "inv_grid_width");
    float fGridHeight = pSettings->r_float(artefact_name, "inv_grid_height");
    float fXPos = pSettings->r_float(artefact_name, "inv_grid_x");
    float fYPos = pSettings->r_float(artefact_name, "inv_grid_y");

    gTexInfo.value().data[ARTEFACT][0].sh = InventoryUtilities::GetEquipmentIconsShader();
    gTexInfo.value().data[ARTEFACT][0].rect.set(fXPos * INV_GRID_WIDTH, fYPos * INV_GRID_HEIGHT,
        fXPos * INV_GRID_WIDTH + fGridWidth * INV_GRID_WIDTH, fYPos * INV_GRID_HEIGHT + fGridHeight * INV_GRID_HEIGHT);

    gTexInfo.value().data[ARTEFACT][1] = gTexInfo.value().data[ARTEFACT][0];

    // death
    gTexInfo.value().data[DEATH][0].sh->create("hud" DELIMITER "default", "ui" DELIMITER "ui_mp_icon_kill");
    gTexInfo.value().data[DEATH][1] = gTexInfo.value().data[DEATH][0];
    gTexInfo.value().data[DEATH][0].rect.x1 = 32;
    gTexInfo.value().data[DEATH][0].rect.y1 = 202;
    gTexInfo.value().data[DEATH][0].rect.x2 = gTexInfo.value().data[DEATH][0].rect.x1 + 30;
    gTexInfo.value().data[DEATH][0].rect.y2 = gTexInfo.value().data[DEATH][0].rect.y1 + 30;
}

void CUIStatsIcon::FreeTexInfo()
{
    // ranks
    for (int i = RANK_0; i <= RANK_4; i++)
    {
        gTexInfo.value().data[i][0].sh->destroy();
        gTexInfo.value().data[i][1].sh->destroy();
    }
    gTexInfo.value().data[ARTEFACT][0].sh->destroy();
    gTexInfo.value().data[ARTEFACT][1].sh->destroy();
    gTexInfo.value().data[DEATH][0].sh->destroy();
    gTexInfo.value().data[DEATH][1].sh->destroy();
}

void CUIStatsIcon::SetValue(LPCSTR str)
{
    if (0 == str[0])
    {
        SetVisible(false);
        return;
    }
    else
        SetVisible(true);

    if (strstr(str, "status"))
    {
        int team = 1;
        if (strstr(str, "green"))
            team = 0;

        const int rank = atoi(strchr(str, '0')) - 1;

        SetShader(gTexInfo.value().data[rank][team].sh);
        SetTextureRect(gTexInfo.value().data[rank][team].rect);
    }
    else if (0 == xr_strcmp(str, "death"))
    {
        SetShader(gTexInfo.value().data[DEATH][0].sh);
        SetTextureRect(gTexInfo.value().data[DEATH][0].rect);
    }
    else if (0 == xr_strcmp(str, "artefact"))
    {
        SetShader(gTexInfo.value().data[ARTEFACT][0].sh);
        SetTextureRect(gTexInfo.value().data[ARTEFACT][0].rect);
    }
    else
    {
        InitTexture(str);
    }
}
