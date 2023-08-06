#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIStatic.h"

class XRUICORE_API CUITextFrameLineWnd final : public CUIWindow
{
    using inherited = CUIWindow;

public:
    CUITextFrameLineWnd();

    void Init(pcstr baseTexture, Fvector2 pos, Fvector2 size, bool horizontal = true);
    void InitTexture(pcstr texture, bool horizontal = true);

    virtual void SetOrientation(bool horizontal)
    {
        m_frameline.SetHorizontal(horizontal);
    }

    float GetTextureHeight() const
    {
        return m_frameline.GetTextureHeight();
    }

    void SetColor(u32 cl)
    {
        m_frameline.SetTextureColor(cl);
    }

    // Also we can display textual caption on the frame
    CUIStatic* GetTitleStatic() { return &m_title; };
    void SetText(pcstr text) { m_title.SetText(text); }

    pcstr GetDebugType() override { return "CUITextFrameLineWnd"; }

protected:
    friend class CUIXmlInitBase;

    bool bHorizontal;
    CUIFrameLineWnd m_frameline;
    CUIStatic m_title;
};
