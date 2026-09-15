#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/XML/xrUIXmlParser.h"
class CUIScrollView;
struct GAME_NEWS_DATA;

class CUINewsWnd : public CUIWindow
{
    typedef CUIWindow inherited;
    enum eFlag
    {
        eNeedAdd = (1 << 0),
    };
    Flags16 m_flags;
    CUIXml uiXml;

public:
    CUINewsWnd();
    virtual ~CUINewsWnd();

    void Init();
    void Init(LPCSTR xml_name, LPCSTR start_from);
    void AddNews();
    void LoadNews();
    virtual void Show(bool status);
    virtual void Update();
    virtual void Reset();

    pcstr GetDebugType() override { return "CUINewsWnd"; }

    CUIScrollView* UIScrollWnd;

private:
    void AddNewsItem(GAME_NEWS_DATA& news_data, bool top = false);
};
