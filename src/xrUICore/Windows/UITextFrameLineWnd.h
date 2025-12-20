#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIStatic.h"

class XRUICORE_API CUITextFrameLineWnd final : public CUIWindow
{
    using inherited = CUIWindow;

public:
    CUITextFrameLineWnd();

    // Also we can display textual caption on the frame
    CUIStatic* GetTitleStatic() { return &m_title; }
    void SetText(pcstr text) { m_title.SetText(text); }

    pcstr GetDebugType() override { return "CUITextFrameLineWnd"; }

protected:
    friend class CUIXmlInitBase;

    CUIFrameLineWnd m_frameline;
    CUIStatic m_title;
};
