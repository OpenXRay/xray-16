#include "stdafx.h"
#include "UIEncyclopediaArticleWnd.h"
#include "UIStatic.h"
#include "../encyclopedia_article.h"
#include "UIXmlInit.h"
#include "../string_table.h"
#include "../../Include/xrRender/UIShader.h"

CUIEncyclopediaArticleWnd::CUIEncyclopediaArticleWnd	()
:m_Article(NULL)
{
}

CUIEncyclopediaArticleWnd::~CUIEncyclopediaArticleWnd	()
{
}

void CUIEncyclopediaArticleWnd::Init(LPCSTR xml_name, LPCSTR start_from)
{
	CUIXml					uiXml;
	uiXml.Load				(CONFIG_PATH, UI_PATH, xml_name);

	CUIXmlInit				xml_init;

	string512				str;

	strcpy_s				(str,sizeof(str),start_from);
	xml_init.InitWindow		(uiXml,str,0,this);

	strconcat				(sizeof(str),str,start_from,":image");
	m_UIImage				= xr_new<CUIStatic>();	m_UIImage->SetAutoDelete(true);
	xml_init.InitStatic			(uiXml,str,0,m_UIImage);
	AttachChild				(m_UIImage);

	strconcat				(sizeof(str),str,start_from,":text_cont");
	m_UIText				= xr_new<CUIStatic>();	m_UIText->SetAutoDelete(true);
	xml_init.InitStatic		(uiXml,str,0,m_UIText);
	AttachChild				(m_UIText);
}

void CUIEncyclopediaArticleWnd::SetArticle(CEncyclopediaArticle* article)
{
	if( article->data()->image.GetShader() && article->data()->image.GetShader()->inited())
	{
		m_UIImage->SetShader			(article->data()->image.GetShader());
		m_UIImage->SetOriginalRect		(article->data()->image.GetStaticItem()->GetOriginalRect());
		m_UIImage->SetWndSize			(article->data()->image.GetWndSize());

		float img_x						= (GetWidth()-m_UIImage->GetWidth())/2.0f;
		img_x							= _max(0.0f, img_x);
		m_UIImage->SetWndPos			(Fvector2().set(img_x ,m_UIImage->GetWndPos().y) );
	};
	m_UIText->SetText					(*CStringTable().translate(article->data()->text.c_str()));
	m_UIText->AdjustHeightToText		();

	AdjustLauout						();
}

void CUIEncyclopediaArticleWnd::AdjustLauout()
{
	m_UIText->SetWndPos					(Fvector2().set(m_UIText->GetWndPos().x, m_UIImage->GetWndPos().y + m_UIImage->GetHeight()));
	SetHeight							(m_UIImage->GetWndPos().y + m_UIImage->GetHeight()+m_UIText->GetHeight());
}

void CUIEncyclopediaArticleWnd::SetArticle(LPCSTR article)
{
	CEncyclopediaArticle				A;
	A.Load								(article);
	SetArticle							(&A);
}
