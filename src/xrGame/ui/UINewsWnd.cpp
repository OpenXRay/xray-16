#include "StdAfx.h"

#include "UINewsWnd.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "xrUICore/ui_base.h"
#include "../HUDManager.h"
#include "../Level.h"
#include "../game_news.h"
#include "../Actor.h"
#include "../alife_registry_wrappers.h"
#include "UIInventoryUtilities.h"
#include "UINewsItemWnd.h"
#include "xrUICore/ScrollView/UIScrollView.h"

#define NEWS_XML "news.xml"
#define NEWS_TO_SHOW 50

CUINewsWnd::CUINewsWnd() : CUIWindow("CUINewsWnd") { m_flags.zero(); }

CUINewsWnd::~CUINewsWnd() {}

void CUINewsWnd::Init(LPCSTR xml_name, LPCSTR start_from)
{
    string512 pth;

    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name);
    R_ASSERT3(xml_result, "xml file not found", xml_name);
    CUIXmlInit xml_init;

    strconcat(sizeof(pth), pth, start_from, "list");
    xml_init.InitWindow(uiXml, pth, 0, this);
    UIScrollWnd = xr_new<CUIScrollView>();
    UIScrollWnd->SetAutoDelete(true);
    AttachChild(UIScrollWnd);
    xml_init.InitScrollView(uiXml, pth, 0, UIScrollWnd);
}

void CUINewsWnd::Init() { Init(NEWS_XML, ""); }

void CUINewsWnd::LoadNews()
{
    UIScrollWnd->Clear();

    if (Actor())
    {
        GAME_NEWS_VECTOR& news_vector = Actor()->game_news_registry->registry().objects();

        // Показать только NEWS_TO_SHOW последних ньюсов
        int currentNews = 0;

        for (GAME_NEWS_VECTOR::reverse_iterator it = news_vector.rbegin(); it != news_vector.rend() && currentNews < NEWS_TO_SHOW; ++it)
        {
            AddNewsItem(*it);
            ++currentNews;
        }
    }
    m_flags.set(eNeedAdd, FALSE);
}

void CUINewsWnd::Update()
{
    inherited::Update();
    if (m_flags.test(eNeedAdd))
        LoadNews();
}

void CUINewsWnd::AddNews()
{
    m_flags.set(eNeedAdd, TRUE);
}

void CUINewsWnd::AddNewsItem(GAME_NEWS_DATA& news_data, bool top)
{
    CUIWindow* itm = NULL;
    switch (news_data.m_type)
    {
    case GAME_NEWS_DATA::eNews: {
        CUINewsItemWnd* _itm = xr_new<CUINewsItemWnd>();
        _itm->Init(uiXml, "news_item");
        _itm->Setup(news_data);
        itm = _itm;
    }
    break;
    case GAME_NEWS_DATA::eTalk: {
        CUINewsItemWnd* _itm = xr_new<CUINewsItemWnd>();
        _itm->Init(uiXml, "talk_item");
        _itm->Setup(news_data);
        itm = _itm;
    }
    break;
    };
    if (itm)
        UIScrollWnd->AddWindow(itm, true);
}

void CUINewsWnd::Show(bool status)
{
    if (status)
    {
        if (m_flags.test(eNeedAdd))
            LoadNews();
    }
    else
    {
        InventoryUtilities::SendInfoToActor("ui_pda_news_hide");
    }

    inherited::Show(status);
}

void CUINewsWnd::Reset()
{
    inherited::Reset();
    UIScrollWnd->Clear();
    m_flags.set(eNeedAdd, TRUE);
}
