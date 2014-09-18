#include "stdafx.h"

#include "UINewsWnd.h"
/*
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../UI.h"
#include "../HUDManager.h"
#include "../level.h"
#include "../game_news.h"
#include "../actor.h"
#include "../alife_registry_wrappers.h"
#include "UIInventoryUtilities.h"
#include "UINewsItemWnd.h"
#include "UIScrollView.h"

#define				NEWS_XML			"news.xml"

#define NEWS_TO_SHOW 50

CUINewsWnd::CUINewsWnd()
{}

CUINewsWnd::~CUINewsWnd()
{}

void CUINewsWnd::Init(LPCSTR xml_name, LPCSTR start_from)
{
	string512 pth;

	CUIXml						uiXml;
	uiXml.Load					(CONFIG_PATH, UI_PATH, xml_name);
	CUIXmlInit					xml_init;

	strconcat					(sizeof(pth),pth,start_from,"list");
	xml_init.InitWindow			(uiXml, pth, 0, this);
	UIScrollWnd					= xr_new<CUIScrollView>();UIScrollWnd->SetAutoDelete(true);
	AttachChild					(UIScrollWnd);
	xml_init.InitScrollView		(uiXml, pth, 0, UIScrollWnd);
}

void CUINewsWnd::Init()
{
	Init				(NEWS_XML,"");
}

void CUINewsWnd::LoadNews()
{
	UIScrollWnd->Clear();

	if (Actor())
	{
		GAME_NEWS_VECTOR& news_vector = Actor()->game_news_registry->registry().objects();
		
		// Показать только NEWS_TO_SHOW последних ньюсов
		int currentNews = 0;

		for (GAME_NEWS_VECTOR::reverse_iterator it = news_vector.rbegin(); it != news_vector.rend() && currentNews < NEWS_TO_SHOW ; ++it)
		{
			AddNewsItem(*it);
			++currentNews;
		}
	}
	m_flags.set(eNeedAdd,FALSE);
}

void CUINewsWnd::Update()
{
	inherited::Update		();	
	if(m_flags.test(eNeedAdd))
		LoadNews			();
}

void CUINewsWnd::AddNews()
{
	m_flags.set(eNeedAdd,TRUE);
}

void CUINewsWnd::AddNewsItem(GAME_NEWS_DATA& news_data)
{
	CUIWindow*				itm = NULL;
	switch(news_data.m_type){
		case GAME_NEWS_DATA::eNews:{
			CUINewsItemWnd* _itm		= xr_new<CUINewsItemWnd>();
			_itm->Init					(NEWS_XML,"news_item");
			_itm->Setup					(news_data);
			itm							= _itm;					   
		}break;
		case GAME_NEWS_DATA::eTalk:{
			CUINewsItemWnd* _itm		= xr_new<CUINewsItemWnd>();
			_itm->Init					(NEWS_XML,"talk_item");
			_itm->Setup					(news_data);
			itm							= _itm;					   
		}break;
	};
	UIScrollWnd->AddWindow	(itm, true);
}


void CUINewsWnd::Show(bool status)
{
	if (status)
		LoadNews();
	else
		InventoryUtilities::SendInfoToActor("ui_pda_news_hide");
	inherited::Show(status);

}
*/