#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUIXml;
class CUIStatic;
class UIHint;
class CUIScrollView;

class CUIRankingsCoC : public CUIWindow
{
    typedef CUIWindow inherited;

private:
    CUIScrollView* m_parent;
    CUIStatic* m_name{};
    CUIStatic* m_descr{};
    CUIStatic* m_icon{};
    //CUIStatic* m_border;
    UIHint* m_hint{};
    u8 m_index{};

public:
    CUIRankingsCoC(CUIScrollView* parent);
    ~CUIRankingsCoC() override;

    void init_from_xml(CUIXml& xml, u8 index, bool bUnique);
    void Update() override;

    void SetName(pcstr name) const;
    void SetDescription(pcstr desc);
    void SetHint(pcstr hint) const;
    void SetIcon(pcstr icon) const;

    virtual void DrawHint();
    void Reset() override;

protected:
    bool ParentHasMe() const;
};
