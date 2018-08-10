#pragma once
#ifdef CALLOFCHERNOBYL_RANKING
#include "xrUICore/Windows/UIWindow.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIHint;
class CUIScrollView;

class CUIRankingsCoC : public CUIWindow
{
    using inherited = CUIWindow;

    CUIScrollView* m_parent;
    CUITextWnd* m_name;
    CUITextWnd* m_descr;
    CUIStatic* m_icon;
    //CUIStatic* m_border;
    UIHint* m_hint;
    u8 m_index;

public:
    CUIRankingsCoC(CUIScrollView* parent);
    virtual ~CUIRankingsCoC();

    void init_from_xml(CUIXml& xml, u8 index, bool bUnique);
    void Update() override;

    void SetName(pcstr name);
    void SetDescription(pcstr desc);
    void SetHint(pcstr hint);
    void SetIcon(pcstr icon);
    void SetFunctor(pcstr func);

    virtual void DrawHint();
    void Reset() override;

protected:
    bool ParentHasMe();
};
#endif
