#include "StdAfx.h"
#include "UIStatsPlayerInfo.h"
#include "xrUICore/Static/UIStatic.h"
#include "game_cl_base.h"
#include "UIStatsIcon.h"
#include "game_cl_artefacthunt.h"
#include "Level.h"
#include "string_table.h"

CUIStatsPlayerInfo::CUIStatsPlayerInfo(xr_vector<PI_FIELD_INFO>* info, CGameFont* pF, u32 text_col)
{
    m_field_info = info;

    m_pF = pF;
    m_text_col = text_col;

    m_pBackground = new CUIStatic();
    AttachChild(m_pBackground);

    R_ASSERT(!info->empty());
}

CUIStatsPlayerInfo::~CUIStatsPlayerInfo()
{
    for (u32 i = 0; i < m_fields.size(); i++)
        xr_delete(m_fields[i]);

    xr_delete(m_pBackground);
}

void CUIStatsPlayerInfo::InitPlayerInfo(Fvector2 pos, Fvector2 size)
{
    CUIWindow::SetWndPos(pos);
    CUIWindow::SetWndSize(size);

    m_pBackground->SetStretchTexture(true);
    m_pBackground->SetWndPos(Fvector2().set(0, 0));
    m_pBackground->SetWndSize(size);
    m_pBackground->InitTexture("ui" DELIMITER "ui_mp_frags_selection");

    xr_vector<PI_FIELD_INFO>& field_info = *m_field_info;
    for (u32 i = 0; i < field_info.size(); i++)
    {
        bool pic;
        if (0 == xr_strcmp(field_info[i].name, "rank"))
            pic = true;
        else if (0 == xr_strcmp(field_info[i].name, "death_atf"))
            pic = true;
        else
            pic = false;

        AddField(field_info[i].width, m_pF, m_text_col, pic);
    }
}

void CUIStatsPlayerInfo::SetInfo(game_PlayerState* pInfo)
{
    m_pPlayerInfo = pInfo;
    if (Level().CurrentViewEntity() && Level().CurrentViewEntity()->ID() == pInfo->GameID)
        m_pBackground->SetVisible(true);
    else
        m_pBackground->SetVisible(false);
}

void CUIStatsPlayerInfo::Update()
{
    if (!m_pPlayerInfo)
        return;

    xr_vector<PI_FIELD_INFO>& field_info = *m_field_info;

    for (u32 i = 0; i < m_fields.size(); i++)
        m_fields[i]->TextItemControl()->SetText(GetInfoByID(*field_info[i].name));

    m_pPlayerInfo = NULL;
}

void CUIStatsPlayerInfo::AddField(float len, CGameFont* pF, u32 text_col, bool icon)
{
    CUIStatic* wnd = icon ? new CUIStatsIcon() : new CUIStatic();
    wnd->SetAutoDelete(true);

    if (m_fields.empty())
    {
        wnd->SetWndPos(Fvector2().set(5, 0));
        wnd->SetWndSize(Fvector2().set(len, this->GetHeight()));
    }
    else
    {
        wnd->SetWndPos(Fvector2().set(m_fields.back()->GetWndRect().right, 0.0f));
        wnd->SetWndSize(Fvector2().set(len, this->GetHeight()));

        wnd->TextItemControl()->SetTextAlignment(CGameFont::alCenter);
    }
    if (pF)
        wnd->TextItemControl()->SetFont(pF);

    wnd->TextItemControl()->SetTextColor(text_col);
    wnd->TextItemControl()->SetTextComplexMode(false);
    m_fields.push_back(wnd);
    AttachChild(wnd);
}

const char* CUIStatsPlayerInfo::GetInfoByID(const char* id)
{
    static string64 ans;

    if (0 == xr_strcmp(id, "name"))
        xr_strcpy(ans, m_pPlayerInfo->getName());
    else if (0 == xr_strcmp(id, "frags"))
        xr_sprintf(ans, "%d", (int)m_pPlayerInfo->frags());
    else if (0 == xr_strcmp(id, "deaths"))
        xr_sprintf(ans, "%d", (int)m_pPlayerInfo->m_iDeaths);
    else if (0 == xr_strcmp(id, "ping"))
        xr_sprintf(ans, "%d", (int)m_pPlayerInfo->ping);
    else if (0 == xr_strcmp(id, "artefacts"))
        xr_sprintf(ans, "%d", (int)m_pPlayerInfo->af_count);
    else if (0 == xr_strcmp(id, "rank"))
    {
        int team = m_pPlayerInfo->team;
        if (GameID() != eGameIDDeathmatch)
            team -= 1;

        if (0 == team)
            xr_sprintf(ans, "ui_hud_status_green_0%d", (int)m_pPlayerInfo->rank + 1);
        else
            xr_sprintf(ans, "ui_hud_status_blue_0%d", (int)m_pPlayerInfo->rank + 1);
    }
    else if (0 == xr_strcmp(id, "death_atf"))
    {
        if (m_pPlayerInfo->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            xr_strcpy(ans, "death");
        else if (GameID() == eGameIDArtefactHunt)
        {
            game_cl_ArtefactHunt* pGameAHunt = smart_cast<game_cl_ArtefactHunt*>(&(Game()));
            R_ASSERT(pGameAHunt);
            if (m_pPlayerInfo->GameID == pGameAHunt->artefactBearerID)
                xr_strcpy(ans, "artefact");
            else
                xr_strcpy(ans, "");
        }
        else
            xr_strcpy(ans, "");
    }
    else if (0 == xr_strcmp(id, "status"))
    {
        if (m_pPlayerInfo->testFlag(GAME_PLAYER_FLAG_READY))
            xr_strcpy(ans, *StringTable().translate("st_mp_ready"));
        else
            xr_strcpy(ans, "");
    }
    else
        R_ASSERT2(false, "invalid info ID");

    return ans;
}
