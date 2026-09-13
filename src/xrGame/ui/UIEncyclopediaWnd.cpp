//=============================================================================
//  Filename:   UIEncyclopediaWnd.cpp
//	Created by Roman E. Marchenko, vortex@gsc-game.kiev.ua
//	Copyright 2004. GSC Game World
//	---------------------------------------------------------------------------
//  Encyclopedia window
//=============================================================================

#include "StdAfx.h"
#include "UIEncyclopediaWnd.h"
#include "UIXmlInit.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "xrUICore/ListWnd/UIListWnd.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UITreeViewItem.h"
#include "UIPdaAux.h"
#include "UIEncyclopediaArticleWnd.h"
#include "../encyclopedia_article.h"
#include "../alife_registry_wrappers.h"
#include "../Actor.h"
#include "Common/object_broker.h"
#include "xrEngine/StringTable/StringTable.h"

#define ENCYCLOPEDIA_DIALOG_XML "encyclopedia.xml"

CUIEncyclopediaWnd::CUIEncyclopediaWnd() : CUIWindow("CUIEncyclopediaWnd") { prevArticlesCount = 0; }

CUIEncyclopediaWnd::~CUIEncyclopediaWnd() { DeleteArticles(); }

void CUIEncyclopediaWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, ENCYCLOPEDIA_DIALOG_XML);
    R_ASSERT3(xml_result, "xml file not found", ENCYCLOPEDIA_DIALOG_XML);

    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main_wnd", 0, this);

    // Load xml data
    UIEncyclopediaIdxBkg = xr_new<CUIFrameWindow>();
    UIEncyclopediaIdxBkg->SetAutoDelete(true);
    AttachChild(UIEncyclopediaIdxBkg);
    xml_init.InitFrameWindow(uiXml, "right_frame_window", 0, UIEncyclopediaIdxBkg);

    xml_init.InitFont(uiXml, "tree_item_font", 0, m_uTreeItemColor, m_pTreeItemFont);
    R_ASSERT(m_pTreeItemFont);
    xml_init.InitFont(uiXml, "tree_root_font", 0, m_uTreeRootColor, m_pTreeRootFont);
    R_ASSERT(m_pTreeRootFont);

    UIEncyclopediaIdxHeader = xr_new<CUIFrameLineWnd>();
    UIEncyclopediaIdxHeader->SetAutoDelete(true);
    UIEncyclopediaIdxBkg->AttachChild(UIEncyclopediaIdxHeader);
    xml_init.InitFrameLine(uiXml, "right_frame_line", 0, UIEncyclopediaIdxHeader);

    if (uiXml.NavigateToNode("a_static"))
    {
        UIAnimation = xr_new<CUIAnimatedStatic>();
        UIAnimation->SetAutoDelete(true);
        UIEncyclopediaIdxHeader->AttachChild(UIAnimation);
        xml_init.InitAnimatedStatic(uiXml, "a_static", 0, UIAnimation);
    }

    UIEncyclopediaInfoBkg = xr_new<CUIFrameWindow>();
    UIEncyclopediaInfoBkg->SetAutoDelete(true);
    AttachChild(UIEncyclopediaInfoBkg);
    xml_init.InitFrameWindow(uiXml, "left_frame_window", 0, UIEncyclopediaInfoBkg);

    UIEncyclopediaInfoHeader = xr_new<CUIFrameLineWnd>();
    UIEncyclopediaInfoHeader->SetAutoDelete(true);
    UIEncyclopediaInfoBkg->AttachChild(UIEncyclopediaInfoHeader);

    xml_init.InitFrameLine(uiXml, "left_frame_line", 0, UIEncyclopediaInfoHeader);
    UIEncyclopediaInfoHeader->GetTitleText(true)->SetEllipsis(true);

    UIArticleHeader = xr_new<CUIStatic>();
    UIArticleHeader->SetAutoDelete(true);
    UIEncyclopediaInfoBkg->AttachChild(UIArticleHeader);
    xml_init.InitStatic(uiXml, "article_header_static", 0, UIArticleHeader);

    UIIdxList = xr_new<CUIListWnd>();
    UIIdxList->SetAutoDelete(true);
    UIEncyclopediaIdxBkg->AttachChild(UIIdxList);
    xml_init.InitListWnd(uiXml, "idx_list", 0, UIIdxList);
    UIIdxList->SetMessageTarget(this);
    UIIdxList->EnableScrollBar(true);

    UIInfoList = xr_new<CUIScrollView>();
    UIInfoList->SetAutoDelete(true);
    UIEncyclopediaInfoBkg->AttachChild(UIInfoList);
    xml_init.InitScrollView(uiXml, "info_list", 0, UIInfoList);

    CUIXmlInit::InitAutoStaticGroup(uiXml, "left_auto_static", 0, UIEncyclopediaInfoBkg);
    CUIXmlInit::InitAutoStaticGroup(uiXml, "right_auto_static", 0, UIEncyclopediaIdxBkg);
}

#include "xrEngine/StringTable/StringTable.h"
void CUIEncyclopediaWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (UIIdxList == pWnd && LIST_ITEM_CLICKED == msg)
    {
        CUITreeViewItem* pTVItem = static_cast<CUITreeViewItem*>(pData);
        R_ASSERT(pTVItem);

        if (pTVItem->vSubItems.size())
        {
            auto& A = m_ArticlesDB[pTVItem->vSubItems[0]->GetValue()];

            xr_string caption = ALL_PDA_HEADER_PREFIX;
            caption += "/";
            caption += CStringTable().translate(A.data()->group).c_str();

            UIEncyclopediaInfoHeader->GetTitleText(true)->SetText(caption.c_str());
            UIArticleHeader->SetTextST(A.data()->group.c_str());
            SetCurrentArtice(NULL);
        }
        else
        {
            auto& A = m_ArticlesDB[pTVItem->GetValue()];
            xr_string caption = ALL_PDA_HEADER_PREFIX;
            caption += "/";
            caption += CStringTable().translate(A.data()->group).c_str();
            caption += "/";
            caption += CStringTable().translate(A.data()->name).c_str();

            UIEncyclopediaInfoHeader->GetTitleText(true)->SetText(caption.c_str());
            SetCurrentArtice(pTVItem);
            UIArticleHeader->SetTextST(A.data()->name.c_str());
        }
    }

    inherited::SendMessage(pWnd, msg, pData);
}

void CUIEncyclopediaWnd::Draw()
{
    UpdateArticles();
    inherited::Draw();
}

void CUIEncyclopediaWnd::ReloadArticles()
{
    if (Actor() && Actor()->encyclopedia_registry->registry().objects_ptr()->size() < prevArticlesCount)
        ResetArticles();
    else
        m_flags.set(eNeedReload, TRUE);
}

void CUIEncyclopediaWnd::Show(bool status)
{
    if (status)
        ReloadArticles();

    inherited::Show(status);
}

bool CUIEncyclopediaWnd::HasArticle(shared_str id)
{
    ReloadArticles();

    for (auto& Art : m_ArticlesDB)
        if (Art.Id() == id)
            return true;

    return false;
}

void CUIEncyclopediaWnd::DeleteArticles()
{
    UIIdxList->RemoveAll();
    m_ArticlesDB.clear();
}

void CUIEncyclopediaWnd::SetCurrentArtice(CUITreeViewItem* pTVItem)
{
    UIInfoList->ScrollToBegin();
    UIInfoList->Clear();

    if (!pTVItem)
        return;

    // для начала проверим, что нажатый элемент не рутовый
    if (!pTVItem->IsRoot())
    {
        CUIEncyclopediaArticleWnd* article_info = xr_new<CUIEncyclopediaArticleWnd>();
        article_info->Init("encyclopedia_item.xml", "encyclopedia_wnd:objective_item");
        article_info->SetArticle(&m_ArticlesDB[pTVItem->GetValue()]);
        UIInfoList->AddWindow(article_info, true);

        // Пометим как прочитанную
        if (!pTVItem->IsArticleReaded())
        {
            if (Actor()->encyclopedia_registry->registry().objects_ptr())
            {
                for (ARTICLE_VECTOR::iterator it = Actor()->encyclopedia_registry->registry().objects().begin(); it != Actor()->encyclopedia_registry->registry().objects().end();
                     it++)
                {
                    if (ARTICLE_DATA::eEncyclopediaArticle == it->article_type && m_ArticlesDB[pTVItem->GetValue()].Id() == it->article_id)
                    {
                        it->readed = true;
                        break;
                    }
                }
            }
        }
    }
}

CEncyclopediaArticle* CUIEncyclopediaWnd::AddArticle(shared_str article_id, bool bReaded)
{
    for (auto& Art : m_ArticlesDB)
        if (Art.Id() == article_id)
            return nullptr;

    // Добавляем элемент
    auto& a = m_ArticlesDB.emplace_back();
    a.Load(article_id);
    return &a;
}

void CUIEncyclopediaWnd::Reset()
{
    inherited::Reset();
    ResetArticles();
}

void CUIEncyclopediaWnd::ResetArticles()
{
    m_flags.set(eNeedReload, TRUE);
    DeleteArticles();
    prevArticlesCount = 0;
}

void CUIEncyclopediaWnd::FillEncyclopedia()
{
    SetCurrentArtice(nullptr);
    UIEncyclopediaInfoHeader->GetTitleText(true)->SetText("");
    UIArticleHeader->SetText("");

    ResetArticles();
    UpdateArticles();
}

void CUIEncyclopediaWnd::UpdateArticles()
{
    if (!m_flags.test(eNeedReload) || !Actor())
        return;

    const ARTICLE_VECTOR* articles = Actor()->encyclopedia_registry->registry().objects_ptr();
    if (!articles)
        return;

    if (articles->size() < prevArticlesCount)
        ResetArticles();

    auto it = articles->begin();
    std::advance(it, prevArticlesCount);
    for (; it != articles->end(); ++it)
    {
        if (it->article_type != ARTICLE_DATA::eEncyclopediaArticle)
            continue;

        CEncyclopediaArticle* article = AddArticle(it->article_id, it->readed);
        if (!article)
            continue;

        CreateTreeBranch(article->data()->group, article->data()->name, UIIdxList, m_ArticlesDB.size() - 1,
            m_pTreeRootFont, m_uTreeRootColor, m_pTreeItemFont, m_uTreeItemColor, it->readed);
    }

    prevArticlesCount = articles->size();
    m_flags.set(eNeedReload, FALSE);
}
