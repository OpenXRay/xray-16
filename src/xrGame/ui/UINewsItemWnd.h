#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/XML/xrUIXmlParser.h"

class CUIStatic;
struct GAME_NEWS_DATA;

class CUINewsItemWnd final : public CUIWindow
{
    typedef CUIWindow inherited;

    CUIStatic* m_UIDate;
    CUIStatic* m_UICaption;
    CUIStatic* m_UIText;
    CUIStatic* m_UIImage;

public:
    CUINewsItemWnd();

    void Init(CUIXml& uiXml, LPCSTR start_from);
    void Setup(GAME_NEWS_DATA& news_data);
    virtual void Update(){};

    pcstr GetDebugType() override { return "CUINewsItemWnd"; }
};
