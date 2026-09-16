#include "StdAfx.h"
#include "UIStalkersRankingWnd.h"
#include "UIXmlInit.h"
#include "UIPdaAux.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "UIPdaListItem.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UICharacterInfo.h"
#include "../InventoryOwner.h"
#include "../Level.h"
#include "../PDA.h"
#include "../Actor.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"

#define STALKERS_RANKING_XML "stalkers_ranking.xml"
#define STALKERS_RANKING_CHARACTER_XML "stalkers_ranking_character.xml"

typedef xr_vector<u16> TOP_LIST;
TOP_LIST g_all_statistic_humans;

CUIStalkersRankingWnd::CUIStalkersRankingWnd() : CUIWindow("CUIStalkersRankingWnd") {}

void CUIStalkersRankingWnd::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, STALKERS_RANKING_XML);

    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main_wnd", 0, this);

    UICharIconFrame = xr_new<CUIFrameWindow>();
    UICharIconFrame->SetAutoDelete(true);
    AttachChild(UICharIconFrame);
    xml_init.InitFrameWindow(uiXml, "chicon_frame_window", 0, UICharIconFrame);

    UICharIconHeader = xr_new<CUIFrameLineWnd>();
    UICharIconHeader->SetAutoDelete(true);
    UICharIconFrame->AttachChild(UICharIconHeader);
    xml_init.InitFrameLine(uiXml, "chicon_frame_line", 0, UICharIconHeader);

    UIInfoFrame = xr_new<CUIFrameWindow>();
    UIInfoFrame->SetAutoDelete(true);
    AttachChild(UIInfoFrame);
    xml_init.InitFrameWindow(uiXml, "info_frame_window", 0, UIInfoFrame);

    UIInfoHeader = xr_new<CUIFrameLineWnd>();
    UIInfoHeader->SetAutoDelete(true);
    UIInfoFrame->AttachChild(UIInfoHeader);
    xml_init.InitFrameLine(uiXml, "info_frame_line", 0, UIInfoHeader);

    if (uiXml.NavigateToNode("a_static"))
    {
        UIAnimatedIcon = xr_new<CUIAnimatedStatic>();
        UIAnimatedIcon->SetAutoDelete(true);
        UIInfoHeader->AttachChild(UIAnimatedIcon);
        xml_init.InitAnimatedStatic(uiXml, "a_static", 0, UIAnimatedIcon);
    }

    UIList = xr_new<CUIScrollView>();
    UIList->SetAutoDelete(true);
    UIInfoFrame->AttachChild(UIList);
    xml_init.InitScrollView(uiXml, "list", 0, UIList);

    UICharacterWindow = xr_new<CUIWindow>();
    UICharacterWindow->SetAutoDelete(true);
    UICharIconFrame->AttachChild(UICharacterWindow);
    xml_init.InitWindow(uiXml, "character_info", 0, UICharacterWindow);

    UICharacterInfo = xr_new<CUICharacterInfo>();
    UICharacterInfo->SetAutoDelete(true);
    UICharacterWindow->AttachChild(UICharacterInfo);
    UICharacterInfo->InitCharacterInfo(Fvector2().set(0, 0), UICharacterWindow->GetWndSize(), STALKERS_RANKING_CHARACTER_XML);

    if (ShadowOfChernobylMode)
    {
        // The SoC layout starts the text at x=165. Leave a small empty column
        // before it instead of drawing the portrait up to the first letter.
        CUIStatic& portrait = UICharacterInfo->UIIcon();
        portrait.SetWidth(portrait.GetWidth() - 10.0f);

        if (CUIStatic* rank = UICharacterInfo->GetIcon(CUICharacterInfo::eRank))
            rank->MoveWndDelta(5.0f, 0.0f);
    }

    xml_init.InitAutoStaticGroup(uiXml, "left_auto", 0, UIInfoFrame);
    xml_init.InitAutoStaticGroup(uiXml, "right_auto", 0, UICharIconFrame);
}

void CUIStalkersRankingWnd::Show(bool status)
{
    inherited::Show(status);
    if (status)
        FillList();
}

extern CSE_ALifeTraderAbstract* ch_info_get_from_id(u16 id);

bool GreaterRankPred(const u16& h1, const u16& h2)
{
    CSE_ALifeTraderAbstract* t1 = ch_info_get_from_id(h1);
    CSE_ALifeTraderAbstract* t2 = ch_info_get_from_id(h2);
    if (t1 && t2)
        return t1->m_rank > t2->m_rank;
    else if (t1)
        return true;
    return false;
}

int get_actor_ranking()
{
    std::sort(g_all_statistic_humans.begin(), g_all_statistic_humans.end(), GreaterRankPred);
    TOP_LIST::iterator it = std::find(g_all_statistic_humans.begin(), g_all_statistic_humans.end(), Actor()->ID());
    if (it != g_all_statistic_humans.end())
        return (int)std::distance(g_all_statistic_humans.begin(), it);
    else
        return 1;
}

void CUIStalkersRankingWnd::FillList()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, STALKERS_RANKING_XML);

    UIList->Clear();

    uiXml.SetLocalRoot(uiXml.NavigateToNode("stalkers_list", 0));

    if (g_all_statistic_humans.size())
    {
        CSE_ALifeTraderAbstract* pActorAbstract = ch_info_get_from_id(Actor()->ID());
        int actor_place = get_actor_ranking();

        int i = 0;
        while (i < 20 && i < g_all_statistic_humans.size())
        {
            u16 id = g_all_statistic_humans[i];
            CSE_ALifeTraderAbstract* pT = ch_info_get_from_id(id);
            if (pT)
            {
                if (pT == pActorAbstract || (i == 19 && actor_place > 19))
                    AddActorItem(&uiXml, actor_place + 1, pActorAbstract);
                else
                    AddStalkerItem(&uiXml, i + 1, pT);
            }
            else
                Msg("! [%s]: i[%d] id[%d] not a CSE_ALifeTraderAbstract", __FUNCTION__, i, id);
            i++;
        }

        UIList->SetSelected(UIList->GetItem(0));
    }
    else
    {
        CUIStalkerRankingInfoItem* itm = xr_new<CUIStalkerRankingInfoItem>(this);
        itm->Init(&uiXml, "no_items", 0);
        UIList->AddWindow(itm, true);
    }
}

void CUIStalkersRankingWnd::ShowHumanInfo(u16 id) { UICharacterInfo->InitCharacter(id); }

void CUIStalkersRankingWnd::AddStalkerItem(CUIXml* xml, int num, CSE_ALifeTraderAbstract* t)
{
    CUIStalkerRankingInfoItem* itm = xr_new<CUIStalkerRankingInfoItem>(this);
    itm->Init(xml, "item_human", 0);

    std::string s = std::to_string(num) + ".";
    itm->m_text1->SetText(s.c_str());

    itm->m_text2->SetText(t->m_character_name.c_str());

    s = std::to_string(t->m_rank);
    itm->m_text3->SetText(s.c_str());

    itm->m_humanID = t->object_id();
    UIList->AddWindow(itm, true);
}

void CUIStalkersRankingWnd::AddActorItem(CUIXml* xml, int num, CSE_ALifeTraderAbstract* t)
{
    CUIStalkerRankingInfoItem* itm;
    if (num > 19)
    {
        itm = xr_new<CUIStalkerRankingElipsisItem>(this);
        itm->Init(xml, "item_ellipsis", 0);
        UIList->AddWindow(itm, true);
    }

    itm = xr_new<CUIStalkerRankingInfoItem>(this);
    itm->Init(xml, "item_actor", 0);

    std::string s = std::to_string(num) + ".";
    itm->m_text1->SetText(s.c_str());

    itm->m_text2->SetText(t->m_character_name.c_str());

    s = std::to_string(t->m_rank);
    itm->m_text3->SetText(s.c_str());

    itm->m_humanID = t->object_id();
    UIList->AddWindow(itm, true);
}

void CUIStalkersRankingWnd::Reset()
{
    inherited::Reset();
    g_all_statistic_humans.clear();
}

void remove_human_from_top_list(u16 id)
{
    TOP_LIST::iterator it = std::find(g_all_statistic_humans.begin(), g_all_statistic_humans.end(), id);
    if (it != g_all_statistic_humans.end())
        g_all_statistic_humans.erase(it);
}

void add_human_to_top_list(u16 id)
{
    CSE_ALifeTraderAbstract* t = ch_info_get_from_id(id);
    if (t)
    {
        remove_human_from_top_list(id);
        g_all_statistic_humans.push_back(id);
    }
    else
        Msg("! [%s]: id[%d] not a CSE_ALifeTraderAbstract", __FUNCTION__, id);
}

CUIStalkerRankingInfoItem::CUIStalkerRankingInfoItem(CUIStalkersRankingWnd* w)
    : CUIWindow("CUIStalkerRankingInfoItem"), m_StalkersRankingWnd(w), m_humanID(u16(-1))
{}

void CUIStalkerRankingInfoItem::Init(CUIXml* xml, LPCSTR path, int idx)
{
    XML_NODE _stored_root = xml->GetLocalRoot();

    CUIXmlInit xml_init;
    xml_init.InitWindow(*xml, path, idx, this);

    xml->SetLocalRoot(xml->NavigateToNode(path, idx));

    m_text1 = xr_new<CUIStatic>();
    m_text1->SetAutoDelete(true);
    AttachChild(m_text1);
    xml_init.InitStatic(*xml, "text_1", 0, m_text1);

    m_text2 = xr_new<CUIStatic>();
    m_text2->SetAutoDelete(true);
    AttachChild(m_text2);
    xml_init.InitStatic(*xml, "text_2", 0, m_text2);

    m_text3 = xr_new<CUIStatic>();
    m_text3->SetAutoDelete(true);
    AttachChild(m_text3);
    xml_init.InitStatic(*xml, "text_3", 0, m_text3);

    xml_init.InitAutoStaticGroup(*xml, "auto", 0, this);

    m_stored_alpha = color_get_A(m_text2->GetTextColor());
    xml->SetLocalRoot(_stored_root);
}

void CUIStalkerRankingInfoItem::SetSelected(bool b)
{
    CUISelectable::SetSelected(b);
    m_text1->SetTextColor(subst_alpha(m_text1->GetTextColor(), b ? 255 : m_stored_alpha));
    m_text2->SetTextColor(subst_alpha(m_text2->GetTextColor(), b ? 255 : m_stored_alpha));
    m_text3->SetTextColor(subst_alpha(m_text3->GetTextColor(), b ? 255 : m_stored_alpha));
    if (b)
    {
        m_StalkersRankingWnd->ShowHumanInfo(m_humanID);
    }
}

bool CUIStalkerRankingInfoItem::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1)
    {
        m_StalkersRankingWnd->GetTopList().SetSelected(this);
        return true;
    }
    else
        return false;
}

CUIStalkerRankingElipsisItem::CUIStalkerRankingElipsisItem(CUIStalkersRankingWnd* w) : inherited(w) {}

void CUIStalkerRankingElipsisItem::SetSelected(bool b) { return; }

bool CUIStalkerRankingElipsisItem::OnMouseDown(int mouse_btn) { return false; }
