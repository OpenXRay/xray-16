#pragma once
#include "xrUICore/Windows/UIFrameWindow.h"

class CUITextWnd;

class XRUICORE_API CUIButtonHint : public CUIFrameWindow
{
    CUIWindow* m_ownerWnd;

    CUITextWnd* m_text;
    bool m_enabledOnFrame;

public:
    CUIButtonHint();
    virtual ~CUIButtonHint();
    CUIWindow* Owner() { return m_ownerWnd; }
    void Discard() { m_ownerWnd = NULL; };
    void OnRender();
    void Draw_() { m_enabledOnFrame = true; };
    void SetHintText(CUIWindow* w, LPCSTR text);
};

XRUICORE_API extern CUIButtonHint* g_btnHint;
XRUICORE_API extern CUIButtonHint* g_statHint;
