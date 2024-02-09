#pragma once
#include "xrUICore/Windows/UIWindow.h"

class CUIXml;
class CUIStatic;
class UIHint;
class CUIScrollView;

class CUIAchievements final : public CUIWindow
{
    typedef CUIWindow inherited;

private:
    CUIScrollView* m_parent{};
    CUIStatic* m_name{};
    CUIStatic* m_descr{};
    CUIStatic* m_icon{};
    UIHint* m_hint{};
    string128 m_functor_str;
    bool m_repeat{};

public:
    CUIAchievements(CUIScrollView* parent);
    virtual ~CUIAchievements();

    void init_from_xml(CUIXml& xml);
    void Update();

    void SetName(LPCSTR name);
    void SetDescription(LPCSTR desc);
    void SetHint(LPCSTR hint);
    void SetIcon(LPCSTR icon);
    void SetFunctor(LPCSTR func);
    void SetRepeatable(bool repeat);

    virtual void DrawHint();
    virtual void Reset();

    pcstr GetDebugType() override { return "CUIAchievements"; }

protected:
    bool ParentHasMe();
};
