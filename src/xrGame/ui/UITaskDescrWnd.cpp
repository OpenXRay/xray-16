#include "stdafx.h"
#include "UITaskDescrWnd.h"
#include "UIXmlInit.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "UIScrollView.h"
#include "UIEncyclopediaArticleWnd.h"
#include "../encyclopedia_article.h"


CUITaskDescrWnd::CUITaskDescrWnd	()
{}

CUITaskDescrWnd::~CUITaskDescrWnd	()
{}

void CUITaskDescrWnd::Init	(CUIXml* doc, LPCSTR start_from)
{
	CUIXmlInit xml_init;
	
	xml_init.InitWindow		(*doc,start_from,0,this);
	
	string512				str;

	m_UIMainFrame			= xr_new<CUIFrameWindow>(); m_UIMainFrame->SetAutoDelete(true);
	AttachChild				(m_UIMainFrame);

	strconcat				(sizeof(str),str,start_from,":main_frame");
	xml_init.InitFrameWindow(*doc,str,0,m_UIMainFrame);

//.	strconcat				(str,start_from,":main_frame");
//.	xml_init.InitAutoStaticGroup(*doc, str, m_UIMainFrame);

	m_UIMainHeader			= xr_new<CUIFrameLineWnd>();m_UIMainHeader->SetAutoDelete(true);
	m_UIMainFrame->AttachChild(m_UIMainHeader);
	strconcat				(sizeof(str),str,start_from,":main_frame:header_frame_line");
	xml_init.InitFrameLine	(*doc,str,0,m_UIMainHeader);

	m_UITaskInfoWnd			= xr_new<CUIScrollView>(); m_UITaskInfoWnd->SetAutoDelete(true);
	m_UIMainFrame->AttachChild(m_UITaskInfoWnd);
	strconcat				(sizeof(str),str,start_from,":main_frame:scroll_view");
	xml_init.InitScrollView	(*doc,str,0,m_UITaskInfoWnd);

}

void CUITaskDescrWnd::Draw						()
{
	inherited::Draw		();
}

void CUITaskDescrWnd::ClearAll					()
{
	m_UITaskInfoWnd->Clear		();
}

void CUITaskDescrWnd::AddArticle				(LPCSTR article)
{
	CUIEncyclopediaArticleWnd*	article_info = xr_new<CUIEncyclopediaArticleWnd>();
	article_info->Init			("encyclopedia_item.xml","events_wnd:objective_item");
	article_info->SetArticle	(article);
	m_UITaskInfoWnd->AddWindow	(article_info, true);
}

void CUITaskDescrWnd::AddArticle					(CEncyclopediaArticle* article)
{
	CUIEncyclopediaArticleWnd*	article_info = xr_new<CUIEncyclopediaArticleWnd>();
	article_info->Init			("encyclopedia_item.xml","events_wnd:objective_item");
	article_info->SetArticle	(article);
	m_UITaskInfoWnd->AddWindow	(article_info, true);
}