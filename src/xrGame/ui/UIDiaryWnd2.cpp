#include "StdAfx.h"
#include "UIDiaryWnd.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "UINewsWnd.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "UIXmlInit.h"
#include "Common/object_broker.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/ListWnd/UIListWnd.h"
#include "UITreeViewItem.h"
#include "UIEncyclopediaArticleWnd.h"
#include "../Level.h"
#include "../Actor.h"
#include "../alife_registry_wrappers.h"
#include "../encyclopedia_article.h"
#include "UIPdaAux.h"

extern u32 g_pda_info_state;

CUIDiaryWnd::CUIDiaryWnd()
{
    m_currFilter = eNone;
    prevArticlesCount = 0;
}

CUIDiaryWnd::~CUIDiaryWnd()
{
    delete_data(m_UINewsWnd);
    delete_data(m_SrcListWnd);
    delete_data(m_DescrView);
    m_ArticlesDB.clear();
    delete_data(m_updatedSectionImage);
    delete_data(m_oldSectionImage);
}

void CUIDiaryWnd::Show(bool status)
{
    inherited::Show(status);
    if (status)
        Reload((EDiaryFilter)m_FilterTab->GetActiveIndex());
}

void RearrangeTabButtons(CUITabControl* pTab, xr_vector<Fvector2>& vec_sign_places);

void CUIDiaryWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "events_new.xml");
    R_ASSERT3(xml_result, "xml file not found", "events_new.xml");
    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main_wnd", 0, this);

    m_UILeftFrame = xr_new<CUIFrameWindow>();
    m_UILeftFrame->SetAutoDelete(true);
    xml_init.InitFrameWindow(uiXml, "main_wnd:left_frame", 0, m_UILeftFrame);
    AttachChild(m_UILeftFrame);

    m_UILeftHeader = xr_new<CUIFrameLineWnd>();
    m_UILeftHeader->SetAutoDelete(true);
    xml_init.InitFrameLine(uiXml, "main_wnd:left_frame:left_frame_header", 0, m_UILeftHeader);
    m_UILeftFrame->AttachChild(m_UILeftHeader);

    m_FilterTab = xr_new<CUITabControl>();
    m_FilterTab->SetAutoDelete(true);
    m_UILeftHeader->AttachChild(m_FilterTab);
    xml_init.InitTabControl(uiXml, "main_wnd:left_frame:left_frame_header:filter_tab", 0, m_FilterTab);
    m_FilterTab->SetWindowName("filter_tab");
    Register(m_FilterTab);
    AddCallbackStr("filter_tab", TAB_CHANGED, fastdelegate::MakeDelegate(this, &CUIDiaryWnd::OnFilterChanged));

    if (uiXml.NavigateToNode("main_wnd:left_frame:left_frame_header:anim_static"))
    {
        m_UIAnimation = xr_new<CUIAnimatedStatic>();
        m_UIAnimation->SetAutoDelete(true);
        xml_init.InitAnimatedStatic(uiXml, "main_wnd:left_frame:left_frame_header:anim_static", 0, m_UIAnimation);
        m_UILeftHeader->AttachChild(m_UIAnimation);
    }

    m_UILeftWnd = xr_new<CUIWindow>();
    m_UILeftWnd->SetAutoDelete(true);
    xml_init.InitWindow(uiXml, "main_wnd:left_frame:work_area", 0, m_UILeftWnd);
    m_UILeftFrame->AttachChild(m_UILeftWnd);

    m_SrcListWnd = xr_new<CUIListWnd>();
    m_SrcListWnd->SetAutoDelete(false);
    xml_init.InitListWnd(uiXml, "main_wnd:left_frame:work_area:src_list", 0, m_SrcListWnd);
    m_SrcListWnd->SetWindowName("src_list");
    Register(m_SrcListWnd);
    AddCallbackStr("src_list", LIST_ITEM_CLICKED, fastdelegate::MakeDelegate(this, &CUIDiaryWnd::OnSrcListItemClicked));

    xml_init.InitFont(uiXml, "main_wnd:left_frame:work_area:src_list:tree_item_font", 0, m_uTreeItemColor, m_pTreeItemFont);
    R_ASSERT(m_pTreeItemFont);
    xml_init.InitFont(uiXml, "main_wnd:left_frame:work_area:src_list:tree_root_font", 0, m_uTreeRootColor, m_pTreeRootFont);
    R_ASSERT(m_pTreeRootFont);

    m_UIRightFrame = xr_new<CUIFrameWindow>();
    m_UIRightFrame->SetAutoDelete(true);
    xml_init.InitFrameWindow(uiXml, "main_wnd:right_frame", 0, m_UIRightFrame);
    AttachChild(m_UIRightFrame);

    m_UIRightHeader = xr_new<CUIFrameLineWnd>();
    m_UIRightHeader->SetAutoDelete(true);
    xml_init.InitFrameLine(uiXml, "main_wnd:right_frame:right_frame_header", 0, m_UIRightHeader);
    m_UIRightFrame->AttachChild(m_UIRightHeader);

    m_UIRightWnd = xr_new<CUIWindow>();
    m_UIRightWnd->SetAutoDelete(true);
    xml_init.InitWindow(uiXml, "main_wnd:right_frame:work_area", 0, m_UIRightWnd);
    m_UIRightFrame->AttachChild(m_UIRightWnd);

    m_UINewsWnd = xr_new<CUINewsWnd>();
    m_UINewsWnd->SetAutoDelete(false);
    m_UINewsWnd->Init();

    m_DescrView = xr_new<CUIScrollView>();
    m_DescrView->SetAutoDelete(false);
    xml_init.InitScrollView(uiXml, "main_wnd:right_frame:work_area:scroll_view", 0, m_DescrView);

    m_updatedSectionImage = xr_new<CUIStatic>();
    xml_init.InitStatic(uiXml, "updated_section_static", 0, m_updatedSectionImage);

    m_oldSectionImage = xr_new<CUIStatic>();
    xml_init.InitStatic(uiXml, "old_section_static", 0, m_oldSectionImage);

    RearrangeTabButtons(m_FilterTab, m_sign_places);
}

void CUIDiaryWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }

void CUIDiaryWnd::OnFilterChanged(CUIWindow* w, void*) { Reload((EDiaryFilter)m_FilterTab->GetActiveIndex()); }

void CUIDiaryWnd::Reload(EDiaryFilter new_filter)
{
    //.	if(m_currFilter==new_filter) return;

    switch (m_currFilter)
    {
    case eJournal: UnloadJournalTab(); break;
    case eNews: UnloadNewsTab(); break;
    };

    m_currFilter = new_filter;

    switch (m_currFilter)
    {
    case eJournal: LoadJournalTab(); break;
    case eNews: LoadNewsTab(); break;
    };
}

void CUIDiaryWnd::AddNews() { m_UINewsWnd->AddNews(); }

void CUIDiaryWnd::MarkNewsAsRead(bool status) {}

void CUIDiaryWnd::UnloadJournalTab()
{
    m_UILeftWnd->DetachChild(m_SrcListWnd);
    m_SrcListWnd->Show(false);

    m_UIRightWnd->DetachChild(m_DescrView);
    m_DescrView->Show(false);
    m_DescrView->Clear();
}

void CUIDiaryWnd::LoadJournalTab()
{
    m_UILeftWnd->AttachChild(m_SrcListWnd);
    m_SrcListWnd->Show(true);

    m_UIRightWnd->AttachChild(m_DescrView);
    m_DescrView->Show(true);

    UpdateJournal();
    g_pda_info_state &= ~pda_section::journal;
}

void CUIDiaryWnd::UnloadNewsTab()
{
    m_UIRightWnd->DetachChild(m_UINewsWnd);
    m_UINewsWnd->Show(false);
}

void CUIDiaryWnd::LoadNewsTab()
{
    m_UIRightWnd->AttachChild(m_UINewsWnd);
    m_UINewsWnd->Show(true);
    g_pda_info_state &= ~pda_section::news;
}

void CUIDiaryWnd::OnSrcListItemClicked(CUIWindow* w, void* p)
{
    CUITreeViewItem* pSelItem = (CUITreeViewItem*)p;
    m_DescrView->Clear();
    if (!pSelItem->IsRoot())
    {
        CUIEncyclopediaArticleWnd* article_info = xr_new<CUIEncyclopediaArticleWnd>();
        article_info->Init("encyclopedia_item.xml", "encyclopedia_wnd:objective_item");
        article_info->SetArticle(&m_ArticlesDB[pSelItem->GetValue()]);
        m_DescrView->AddWindow(article_info, true);

        // Исправление отображения зеленым цветом прочитанных записей в дневнике КПК
        if (!pSelItem->IsArticleReaded())
        {
            if (Actor()->encyclopedia_registry->registry().objects_ptr())
            {
                for (ARTICLE_VECTOR::iterator it = Actor()->encyclopedia_registry->registry().objects().begin(); it != Actor()->encyclopedia_registry->registry().objects().end();
                     it++)
                {
                    if (ARTICLE_DATA::eJournalArticle == it->article_type && m_ArticlesDB[pSelItem->GetValue()].Id() == it->article_id)
                    {
                        it->readed = true;
                        break;
                    }
                }
            }
        }
    }
}

void draw_sign(CUIStatic* s, Fvector2& pos);
void CUIDiaryWnd::Draw()
{
    inherited::Draw();

    m_updatedSectionImage->Update();
    m_oldSectionImage->Update();

    Fvector2 tab_pos;
    m_FilterTab->GetAbsolutePos(tab_pos);

    Fvector2 pos;

    pos = m_sign_places[eNews];
    pos.add(tab_pos);
    if (g_pda_info_state & pda_section::news)
        draw_sign(m_updatedSectionImage, pos);
    else
        draw_sign(m_oldSectionImage, pos);

    pos = m_sign_places[eJournal];
    pos.add(tab_pos);
    if (g_pda_info_state & pda_section::journal)
        draw_sign(m_updatedSectionImage, pos);
    else
        draw_sign(m_oldSectionImage, pos);
}

void CUIDiaryWnd::Reset()
{
    inherited::Reset();
    m_UINewsWnd->Reset();
    ResetJournal();
}

void CUIDiaryWnd::FillNews()
{
    m_UINewsWnd->LoadNews();
    UpdateJournal();
}

void CUIDiaryWnd::ReloadJournal()
{
    if (Actor() && Actor()->encyclopedia_registry->registry().objects_ptr()->size() < prevArticlesCount)
        ResetJournal();
}

void CUIDiaryWnd::ResetJournal()
{
    m_SrcListWnd->RemoveAll();
    m_ArticlesDB.clear();
    prevArticlesCount = 0;
}

void CUIDiaryWnd::UpdateJournal()
{
    if (Actor()->encyclopedia_registry->registry().objects_ptr() && Actor()->encyclopedia_registry->registry().objects_ptr()->size() > prevArticlesCount)
    {
        ARTICLE_VECTOR::const_iterator it = Actor()->encyclopedia_registry->registry().objects_ptr()->begin();
        std::advance(it, prevArticlesCount);
        for (; it != Actor()->encyclopedia_registry->registry().objects_ptr()->end(); it++)
        {
            if (it->article_type == ARTICLE_DATA::eJournalArticle)
            {
                auto& a = m_ArticlesDB.emplace_back();
                a.Load(it->article_id);
                bool bReaded = it->readed;
                CreateTreeBranch(a.data()->group, a.data()->name, m_SrcListWnd, m_ArticlesDB.size() - 1, m_pTreeRootFont, m_uTreeRootColor, m_pTreeItemFont, m_uTreeItemColor,
                                 bReaded);
            }
        }
        prevArticlesCount = Actor()->encyclopedia_registry->registry().objects_ptr()->size();
    }
}
