#include "StdAfx.h"
#include "UIStatsPlayerList.h"
#include "game_cl_base.h"
#include "game_cl_artefacthunt.h"
#include "UIStatsPlayerInfo.h"
#include "UIStatsIcon.h"
#include "string_table.h"
#include "Level.h"
#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"

IC bool DM_Compare_Players(game_PlayerState* p1, game_PlayerState* p2);

CUIStatsPlayerList::CUIStatsPlayerList()
{
    m_CurTeam = 0;
    m_bSpectator = false;
    m_bStatus_mode = false;

    m_header = new CUIStatic();
    m_header_team = NULL;
    m_header_text = NULL;
    m_i.c = 0xff000000;
    m_i.f = NULL;
    m_i.h = 16;
    m_h = m_i;
    m_t = m_i;
    m_prev_upd_time = 0;
}

CUIStatsPlayerList::~CUIStatsPlayerList() { CUIStatsIcon::FreeTexInfo(); }
void CUIStatsPlayerList::AddField(const char* name, float width)
{
    PI_FIELD_INFO fi;
    fi.name = name;
    fi.width = width;
    m_field_info.push_back(fi);
}

void CUIStatsPlayerList::Init(CUIXml& xml_doc, LPCSTR path)
{
    // init window
    CUIXmlInit::InitScrollView(xml_doc, path, 0, this);
    SetFixedScrollBar(false);

    m_bStatus_mode = xml_doc.ReadAttribInt(path, 0, "status_mode", 0) ? true : false;
    m_bSpectator = xml_doc.ReadAttribInt(path, 0, "spectator", 0) ? true : m_bStatus_mode;
    SetSpectator(m_bSpectator);

    // init item structure
    int tabsCount = xml_doc.GetNodesNum(path, 0, "field");
    XML_NODE tab_node = xml_doc.NavigateToNode(path, 0);
    xml_doc.SetLocalRoot(tab_node);

    for (int i = 0; i < tabsCount; ++i)
    {
        LPCSTR name = xml_doc.ReadAttrib("field", i, "name");
        float width = xml_doc.ReadAttribFlt("field", i, "width");

        if (0 == xr_strcmp(name, "artefacts") && GameID() != eGameIDArtefactHunt)
            continue;

        AddField(name, width);
    }
    xml_doc.SetLocalRoot(xml_doc.GetRoot());
    string256 _path;
    // init item text params
    CUIXmlInit::InitFont(xml_doc, strconcat(sizeof(_path), _path, path, ":text_format"), 0, m_i.c, m_i.f);
    m_i.h = xml_doc.ReadAttribFlt(strconcat(sizeof(_path), _path, path, ":text_format"), 0, "height", 25);

    // init list header
    switch (GameID())
    {
    case eGameIDCaptureTheArtefact:
    case eGameIDArtefactHunt:
    case eGameIDTeamDeathmatch:
        if (!m_bSpectator || m_bStatus_mode)
            InitTeamHeader(xml_doc, path);
    case eGameIDDeathmatch: InitHeader(xml_doc, path);
    default: break;
    }
}

LPCSTR CUIStatsPlayerList::GetST_entry(LPCSTR itm)
{
    static LPCSTR mp_name = "mp_name";
    static LPCSTR mp_frags = "mp_frags";
    static LPCSTR mp_deaths = "mp_deaths";
    static LPCSTR mp_ping = "mp_ping";
    static LPCSTR mp_artefacts = "mp_artefacts";
    static LPCSTR mp_status = "mp_status";

    if (0 == xr_strcmp(itm, "name"))
        return mp_name;
    else if (0 == xr_strcmp(itm, "frags"))
        return mp_frags;
    else if (0 == xr_strcmp(itm, "deaths"))
        return mp_deaths;
    else if (0 == xr_strcmp(itm, "ping"))
        return mp_ping;
    else if (0 == xr_strcmp(itm, "artefacts"))
        return mp_artefacts;
    else if (0 == xr_strcmp(itm, "status"))
        return mp_status;
    else
        NODEFAULT;

#ifdef DEBUG
    return NULL;
#endif // DEBUG
}

void CUIStatsPlayerList::InitHeader(CUIXml& xml_doc, LPCSTR path)
{
    string256 _path;
    CUIXmlInit::InitStatic(xml_doc, strconcat(sizeof(_path), _path, path, ":list_header"), 0, m_header);
    m_header->SetWidth(this->GetDesiredChildWidth());
    m_h.h = m_header->GetHeight();

    CUIXmlInit::InitFont(xml_doc, strconcat(sizeof(_path), _path, path, ":list_header:text_format"), 0, m_h.c, m_h.f);
    float indent = 5;
    if (!m_bSpectator || m_bStatus_mode)
    {
        for (u32 i = 0; i < m_field_info.size(); ++i)
        {
            CUITextWnd* st = new CUITextWnd();
            st->SetAutoDelete(true);
            st->SetWndPos(Fvector2().set(indent, 10.0f));
            st->SetWndSize(Fvector2().set(m_field_info[i].width, m_header->GetHeight()));
            indent += m_field_info[i].width;

            if (0 == xr_strcmp(m_field_info[i].name, "rank"))
                st->SetText("");
            else if (0 == xr_strcmp(m_field_info[i].name, "death_atf"))
                st->SetText("");
            else
            {
                st->SetTextST(GetST_entry(*m_field_info[i].name));
            }

            if (m_h.f)
                st->SetFont(m_h.f);
            st->SetTextColor(m_h.c);
            st->SetTextComplexMode(false);
            if (0 != i)
                st->SetTextAlignment(CGameFont::alCenter);
            m_header->AttachChild(st);
        }
    }
    else
    {
        CUITextWnd* st = new CUITextWnd();
        st->SetAutoDelete(true);
        st->SetWndPos(Fvector2().set(10, 0));
        st->SetWndSize(Fvector2().set(this->GetDesiredChildWidth(), m_h.h));
        if (m_h.f)
            st->SetFont(m_h.f);

        st->SetTextColor(m_h.c);
        st->SetVTextAlignment(valCenter);
        st->SetTextComplexMode(false);
        st->SetTextST("mp_spectators");
        m_header->AttachChild(st);
    }
}

void CUIStatsPlayerList::InitTeamHeader(CUIXml& xml_doc, LPCSTR path)
{
    string256 _path;
    m_header_team = new CUIWindow();
    m_header_team->SetAutoDelete(true);
    CUIXmlInit::InitWindow(xml_doc, strconcat(sizeof(_path), _path, path, ":team_header"), 0, m_header_team);
    m_header_team->SetWidth(this->GetDesiredChildWidth());

    CUIStatic* logo = new CUIStatic();
    logo->SetAutoDelete(true);
    CUIXmlInit::InitStatic(xml_doc, strconcat(sizeof(_path), _path, path, ":team_header:logo"), 0, logo);
    m_header_team->AttachChild(logo);

    if (1 == m_CurTeam)
        logo->InitTexture(pSettings->r_string("team_logo_small", "team1"));
    else if (2 == m_CurTeam)
        logo->InitTexture(pSettings->r_string("team_logo_small", "team2"));
    else
        R_ASSERT2(false, "invalid team");

    S_ELEMENT t;
    CUIXmlInit::InitFont(xml_doc, strconcat(sizeof(_path), _path, path, ":team_header:text_format"), 0, t.c, t.f);
    t.h = m_header_team->GetHeight();

    m_header_text = new CUITextWnd();
    m_header_text->SetAutoDelete(true);
    CUIXmlInit::InitTextWnd(xml_doc, strconcat(sizeof(_path), _path, path, ":team_header:header"), 0, m_header_text);
    m_header_text->SetWidth(GetDesiredChildWidth());
    m_header_text->SetVTextAlignment(valCenter);
    m_header_team->AttachChild(m_header_text);
    if (t.f)
        m_header_text->SetFont(t.f);
    m_header_text->SetTextColor(t.c);
}

void CUIStatsPlayerList::Update()
{
    static string512 teaminfo;
    if (m_prev_upd_time > Device.dwTimeContinual - 100)
        return;

    using ItemVec = xr_vector<game_PlayerState*>;
    ItemVec items;

    m_prev_upd_time = Device.dwTimeContinual;
    game_cl_GameState::PLAYERS_MAP_IT I = Game().players.begin();
    game_cl_GameState::PLAYERS_MAP_IT E = Game().players.end();

    items.clear();
    u32 pl_count = 0;
    int pl_frags = 0;
    u32 pl_artefacts = 0;
    for (; I != E; ++I)
    {
        game_PlayerState* p = (game_PlayerState*)I->second;
        if (!p || p->team != m_CurTeam)
            continue;
        if (m_bStatus_mode || m_bSpectator && p->testFlag(GAME_PLAYER_FLAG_SPECTATOR) ||
            !m_bSpectator && !p->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
        {
            items.push_back(I->second);
            // add to team info
            pl_frags += p->frags();
        }
    };
    pl_count = items.size();

    if (GameID() == eGameIDArtefactHunt && !m_bSpectator)
    {
        game_cl_ArtefactHunt* game = static_cast<game_cl_ArtefactHunt*>(&Game());
        pl_artefacts = game->teams[m_CurTeam - 1].score;
        xr_sprintf(teaminfo, "%s: %u, %s: %u, %s: %d", *StringTable().translate("mp_artefacts_upcase"), pl_artefacts,
            *StringTable().translate("mp_players"), pl_count, *StringTable().translate("mp_frags_upcase"), pl_frags);
        m_header_text->SetText(teaminfo);
    }
    else if (GameID() == eGameIDTeamDeathmatch && !m_bSpectator)
    {
        game_cl_TeamDeathmatch* game = static_cast<game_cl_TeamDeathmatch*>(&Game());
        pl_frags = game->teams[m_CurTeam - 1].score;
        xr_sprintf(teaminfo, "%s: %d, %s: %u", *StringTable().translate("mp_frags_upcase"), pl_frags,
            *StringTable().translate("mp_players"), pl_count);
        m_header_text->SetText(teaminfo);
    }

    if (m_bSpectator)
    {
        if (items.empty())
        {
            Clear();
            ShowHeader(false);
            return;
        }
        else
            ShowHeader(true);
    }

    std::sort(items.begin(), items.end(), DM_Compare_Players);

    int n = (int)items.size();
    n -= m_pad->GetChildWndList().size();

    if (n < 0)
    {
        n = abs(n);
        for (int i = 0; i < n; i++)
            m_pad->DetachChild(*(m_pad->GetChildWndList().begin()));
        m_flags.set(eNeedRecalc, TRUE);
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            CUIStatsPlayerInfo* pi = new CUIStatsPlayerInfo(&m_field_info, m_i.f, m_i.c);
            pi->InitPlayerInfo(Fvector2().set(0, 0), Fvector2().set(this->GetDesiredChildWidth(), m_i.h));
            CUIScrollView::AddWindow(pi, true);
            m_flags.set(eNeedRecalc, TRUE);
        }
    }

    R_ASSERT(items.size() == m_pad->GetChildWndList().size());

    auto it = m_pad->GetChildWndList().begin();
    auto itit = items.begin();

    for (; it != m_pad->GetChildWndList().end(); it++, itit++)
    {
        CUIStatsPlayerInfo* pi = smart_cast<CUIStatsPlayerInfo*>(*it);
        R_ASSERT(pi);
        game_PlayerState* ps = static_cast<game_PlayerState*>(*itit);
        pi->SetInfo(ps);
    }

    // update player info

    CUIScrollView::Update();
}

void CUIStatsPlayerList::SetSpectator(bool f) { m_bSpectator = f; }
void CUIStatsPlayerList::SetTeam(int team) { m_CurTeam = team; }
void CUIStatsPlayerList::AddWindow(CUIWindow* pWnd, bool auto_delete) {}
CUIStatic* CUIStatsPlayerList::GetHeader() { return m_header; }
CUIWindow* CUIStatsPlayerList::GetTeamHeader() { return m_header_team; }
void CUIStatsPlayerList::RecalcSize()
{
    CUIScrollView::RecalcSize();
    if (GetHeight() < m_pad->GetHeight())
    {
        SetHeight(m_pad->GetHeight());
        GetMessageTarget()->SendMessage(this, CHILD_CHANGED_SIZE, NULL);
    }
}

void CUIStatsPlayerList::ShowHeader(bool bShow)
{
    if (m_header)
    {
        m_header->Show(bShow);
        m_header->SetHeight(bShow ? m_h.h : 0);
    }
    m_flags.set(eNeedRecalc, TRUE);
}
