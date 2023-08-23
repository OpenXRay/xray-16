#include "StdAfx.h"
#include "UIActorInfo.h"
#include "UIXmlInit.h"

#include "xrUICore/Windows/UITextFrameLineWnd.h"
#include "xrUICore/Static/UIAnimatedStatic.h"

#include "Actor.h"

#include "UIInventoryUtilities.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"
#include "actor_statistic_mgr.h"
#include "character_community.h"
#include "character_reputation.h"
#include "relation_registry.h"

constexpr cpcstr ACTOR_STATISTIC_XML = "actor_statistic.xml";
constexpr cpcstr ACTOR_CHARACTER_XML = "pda_dialog_character.xml";

CUIActorInfoWnd::CUIActorInfoWnd() : CUIWindow(CUIActorInfoWnd::GetDebugType()) {}

bool CUIActorInfoWnd::Init()
{
    CUIXml uiXml;
    if (!uiXml.Load(CONFIG_PATH, UI_PATH, ACTOR_STATISTIC_XML, false))
        return false;

    CUIXmlInit::InitWindow(uiXml, "main_wnd", 0, this);

    // Декоративное оформление
    UICharIconFrame = xr_new<CUIFrameWindow>("Character icon frame");
    UICharIconFrame->SetAutoDelete(true);
    CUIXmlInit::InitFrameWindow(uiXml, "chicon_frame_window", 0, UICharIconFrame);
    AttachChild(UICharIconFrame);

    UICharIconHeader = xr_new<CUITextFrameLineWnd>();
    UICharIconHeader->SetAutoDelete(true);
    CUIXmlInit::InitTextFrameLine(uiXml, "chicon_frame_line", 0, UICharIconHeader);
    UICharIconFrame->AttachChild(UICharIconHeader);

    UIAnimatedIcon = xr_new<CUIAnimatedStatic>();
    UIAnimatedIcon->SetAutoDelete(true);
    CUIXmlInit::InitAnimatedStatic(uiXml, "a_static", 0, UIAnimatedIcon);
    UICharIconHeader->AttachChild(UIAnimatedIcon);

    UIInfoFrame = xr_new<CUIFrameWindow>("Info frame");
    UIInfoFrame->SetAutoDelete(true);
    CUIXmlInit::InitFrameWindow(uiXml, "info_frame_window", 0, UIInfoFrame);
    AttachChild(UIInfoFrame);

    UIInfoHeader = xr_new<CUITextFrameLineWnd>();
    UIInfoHeader->SetAutoDelete(true);
    CUIXmlInit::InitTextFrameLine(uiXml, "info_frame_line", 0, UIInfoHeader);
    UIInfoFrame->AttachChild(UIInfoHeader);

    UIDetailList = xr_new<CUIScrollView>();
    UIDetailList->SetAutoDelete(true);
    UIInfoFrame->AttachChild(UIDetailList);
    CUIXmlInit::InitScrollView(uiXml, "detail_list", 0, UIDetailList);

    UIMasterList = xr_new<CUIScrollView>();
    UIMasterList->SetAutoDelete(true);
    UICharIconFrame->AttachChild(UIMasterList);
    CUIXmlInit::InitScrollView(uiXml, "master_list", 0, UIMasterList);

    UICharacterWindow = xr_new<CUIWindow>("Character window");
    UICharacterWindow->SetAutoDelete(true);
    UICharIconFrame->AttachChild(UICharacterWindow);
    CUIXmlInit::InitWindow(uiXml, "character_info", 0, UICharacterWindow);

    UICharacterInfo = xr_new<CUICharacterInfo>();
    UICharacterInfo->SetAutoDelete(true);
    UICharacterWindow->AttachChild(UICharacterInfo);
    UICharacterInfo->InitCharacterInfo(UICharacterWindow->GetWndPos(), UICharacterWindow->GetWndSize(), ACTOR_CHARACTER_XML);

    // Элементы автоматического добавления
    CUIXmlInit::InitAutoStaticGroup(uiXml, "right_auto_static", 0, UICharIconFrame);
    CUIXmlInit::InitAutoStaticGroup(uiXml, "left_auto_static", 0, UIInfoFrame);

    return true;
}

void CUIActorInfoWnd::Show(bool status)
{
    inherited::Show(status);
    if (!status) return;

    UICharacterInfo->InitCharacter(Actor()->ID());
    UICharIconHeader->SetText(Actor()->Name());
    FillPointsInfo();
}


void CUIActorInfoWnd::FillPointsInfo()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH,ACTOR_STATISTIC_XML);

    UIMasterList->Clear();

#ifndef PRIQUEL
    const int items_num = uiXml.GetNodesNum("actor_stats_wnd", 0, "master_part");
    uiXml.SetLocalRoot(uiXml.NavigateToNode("actor_stats_wnd", 0));
    string64 buff;

    for (int i = 0; i < items_num; ++i)
    {
        CUIActorStaticticHeader* itm = xr_new<CUIActorStaticticHeader>(this);
        itm->Init(&uiXml, "master_part", i);

        if (itm->m_id != "foo")
        {
            if (itm->m_id == "reputation")
            {
                itm->m_text2->SetTextST(InventoryUtilities::GetReputationAsText(Actor()->Reputation()));
                itm->m_text2->TextItemControl()->SetTextColor(InventoryUtilities::GetReputationColor(Actor()->Reputation()));
            }
            else
            {
                const s32 _totl = Actor()->StatisticMgr().GetSectionPoints(itm->m_id);

                if (_totl == -1)
                {
                    itm->m_text2->SetTextST("");
                }
                else
                {
                    xr_sprintf(buff, "%d", _totl);
                    itm->m_text2->SetTextST(buff);
                }
            }
        }
        UIMasterList->AddWindow(itm, true);
    }
#else
    const vStatSectionData& _storage = Actor()->StatisticMgr().GetCStorage();
    vStatSectionData::const_iterator it = _storage.begin();
    vStatSectionData::const_iterator it_e = _storage.end();

    FillMasterPart(&uiXml, "foo");

    for (; it != it_e; ++it)
    {
        FillMasterPart(&uiXml, (*it).key);
    }
    FillMasterPart(&uiXml, "total");
#endif
    UIMasterList->SetSelected(UIMasterList->GetItem(1));
}

void CUIActorInfoWnd::FillMasterPart(CUIXml* xml, const shared_str& key_name)
{
    CUIActorStaticticHeader* itm = xr_new<CUIActorStaticticHeader>(this);
    string128 buff;
    strconcat(sizeof(buff), buff, "actor_stats_wnd:master_part_", key_name.c_str());
    itm->Init(xml, buff, 0);

    if (key_name != "foo")
    {
        if (key_name == "reputation")
        {
            itm->m_text2->SetTextST(InventoryUtilities::GetReputationAsText(Actor()->Reputation()));
            itm->m_text2->TextItemControl()->SetTextColor(InventoryUtilities::GetReputationColor(Actor()->Reputation()));
        }
        else
        {
            const s32 _totl = Actor()->StatisticMgr().GetSectionPoints(key_name);

            if (_totl == -1)
            {
                itm->m_text2->SetTextST("");
            }
            else
            {
                xr_sprintf(buff, "%d", _totl);
                itm->m_text2->SetTextST(buff);
            }
        }
    }
    UIMasterList->AddWindow(itm, true);
}

void CUIActorInfoWnd::FillPointsDetail(const shared_str& id)
{
    UIDetailList->Clear();
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH,ACTOR_STATISTIC_XML);
    uiXml.SetLocalRoot(uiXml.NavigateToNode("actor_stats_wnd", 0));

    string512 path;
    xr_sprintf(path, "detail_part_%s", id.c_str());

    const XML_NODE n = uiXml.NavigateToNode(path, 0);
    if (!n)
        xr_sprintf(path, "detail_part_def");

    if (id == "reputation") //reputation
    {
        UIInfoHeader->GetTitleStatic()->SetTextST("st_detail_list_for_community_relations");
        FillReputationDetails(&uiXml, path);
        return;
    }
    string256 str;
    xr_sprintf(str, "st_detail_list_for_%s", id.c_str());
    UIInfoHeader->GetTitleStatic()->SetTextST(str);

    SStatSectionData& section = Actor()->StatisticMgr().GetSection(id);
    vStatDetailData::const_iterator it = section.data.begin();
    const vStatDetailData::const_iterator it_e = section.data.end();

    int _cntr = 0;
    string64 buff;
    for (; it != it_e; ++it, ++_cntr)
    {
        CUIActorStaticticDetail* itm = xr_new<CUIActorStaticticDetail>();
        itm->Init(&uiXml, path, 0);

        xr_sprintf(buff, "%d.", _cntr);
        itm->m_text0->SetText(buff);

        itm->m_text1->SetTextST(*CStringTable().translate((*it).key));
        itm->m_text1->AdjustHeightToText();

        if (0 == (*it).str_value.size())
        {
            xr_sprintf(buff, "x%d", (*it).int_count);
            itm->m_text2->SetTextST(buff);

            xr_sprintf(buff, "%d", (*it).int_points);
            itm->m_text3->SetTextST(buff);
        }
        else
        {
            itm->m_text2->SetTextST((*it).str_value.c_str());
            itm->m_text3->SetTextST("");
        }

        Fvector2 sz = itm->GetWndSize();
        const float _height = _max(sz.y, itm->m_text1->GetWndPos().y + itm->m_text1->GetWndSize().y + 3);
        sz.y = _height;
        itm->SetWndSize(sz);
        UIDetailList->AddWindow(itm, true);
    }
}

void CUIActorInfoWnd::Reset()
{
    inherited::Reset();
}

void CUIActorInfoWnd::FillReputationDetails(CUIXml* xml, LPCSTR path)
{
    const XML_NODE _list_node = xml->NavigateToNode("relation_communities_list", 0);
    const int cnt = xml->GetNodesNum("relation_communities_list", 0, "r");

    CHARACTER_COMMUNITY comm;
    CHARACTER_REPUTATION rep_actor, rep_neutral;

    rep_actor.set(Actor()->Reputation());
    rep_neutral.set(NEUTAL_REPUTATION);

    const CHARACTER_GOODWILL d_neutral = CHARACTER_REPUTATION::relation(rep_actor.index(), rep_neutral.index());

    string64 buff;
    for (int i = 0; i < cnt; ++i)
    {
        CUIActorStaticticDetail* itm = xr_new<CUIActorStaticticDetail>();
        itm->Init(xml, path, 0);
        comm.set(xml->Read(_list_node, "r", i, "unknown_community"));
        itm->m_text1->SetTextST(*(comm.id()));

        CHARACTER_GOODWILL gw = RELATION_REGISTRY().GetCommunityGoodwill(comm.index(), Actor()->ID());
        gw += CHARACTER_COMMUNITY::relation(Actor()->Community(), comm.index());
        gw += d_neutral;

        itm->m_text2->SetTextST(InventoryUtilities::GetGoodwillAsText(gw));
        itm->m_text2->TextItemControl()->SetTextColor(InventoryUtilities::GetGoodwillColor(gw));

        xr_sprintf(buff, "%d", gw);
        itm->m_text3->SetTextST(buff);

        UIDetailList->AddWindow(itm, true);
    }
}


CUIActorStaticticHeader::CUIActorStaticticHeader(CUIActorInfoWnd* w)
    : CUIWindow(CUIActorStaticticHeader::GetDebugType()), m_actorInfoWnd(w),
      m_stored_alpha(0), m_text1(nullptr), m_text2(nullptr) {}

void CUIActorStaticticHeader::Init(CUIXml* xml, LPCSTR path, int idx_in_xml)
{
    const XML_NODE _stored_root = xml->GetLocalRoot();

    CUIXmlInit::InitWindow(*xml, path, idx_in_xml, this);

    xml->SetLocalRoot(xml->NavigateToNode(path, idx_in_xml));

    m_text1 = xr_new<CUIStatic>("Text 1");
    m_text1->SetAutoDelete(true);
    AttachChild(m_text1);
    CUIXmlInit::InitStatic(*xml, "text_1", 0, m_text1);

    m_text2 = xr_new<CUIStatic>("Text 2");
    m_text2->SetAutoDelete(true);
    AttachChild(m_text2);
    CUIXmlInit::InitStatic(*xml, "text_2", 0, m_text2);

    CUIXmlInit::InitAutoStaticGroup(*xml, "auto", 0, this);

#ifndef PRIQUEL
    m_id = xml->ReadAttrib(xml->GetLocalRoot(), "id", nullptr);
#else
    pcstr _id = strstr(path, "master_part_") + xr_strlen("master_part_");
    m_id = _id;
#endif

    m_stored_alpha = color_get_A(m_text1->TextItemControl()->GetTextColor());
    xml->SetLocalRoot(_stored_root);
}

bool CUIActorStaticticHeader::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1 && m_id != "total")
    {
        m_actorInfoWnd->MasterList().SetSelected(this);
        return true;
    }
    return true;
}

void CUIActorStaticticHeader::SetSelected(bool b)
{
    CUISelectable::SetSelected(b);
    m_text1->TextItemControl()->SetTextColor(subst_alpha(m_text1->TextItemControl()->GetTextColor(), b ? 255 : m_stored_alpha));
    if (b)
    {
        m_actorInfoWnd->FillPointsDetail(m_id);
    }
}


void CUIActorStaticticDetail::Init(CUIXml* xml, LPCSTR path, int idx)
{
    const XML_NODE _stored_root = xml->GetLocalRoot();

    CUIXmlInit::InitWindow(*xml, path, idx, this);

    xml->SetLocalRoot(xml->NavigateToNode(path, idx));

    m_text0 = xr_new<CUIStatic>("Text 0");
    m_text0->SetAutoDelete(true);
    AttachChild(m_text0);
    CUIXmlInit::InitStatic(*xml, "text_0", 0, m_text0);

    m_text1 = xr_new<CUIStatic>("Text 1");
    m_text1->SetAutoDelete(true);
    AttachChild(m_text1);
    CUIXmlInit::InitStatic(*xml, "text_1", 0, m_text1);

    m_text2 = xr_new<CUIStatic>("Text 2");
    m_text2->SetAutoDelete(true);
    AttachChild(m_text2);
    CUIXmlInit::InitStatic(*xml, "text_2", 0, m_text2);

    m_text3 = xr_new<CUIStatic>("Text 3");
    m_text3->SetAutoDelete(true);
    AttachChild(m_text3);
    CUIXmlInit::InitStatic(*xml, "text_3", 0, m_text3);

    CUIXmlInit::InitAutoStaticGroup(*xml, "auto", 0, this);

    xml->SetLocalRoot(_stored_root);
}
