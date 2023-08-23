#pragma once
#include "xrUICore/Windows/UIFrameWindow.h"

class CUITextWnd;

class XRUICORE_API CUIButtonHint final : public CUIFrameWindow
{
    CUIWindow* m_ownerWnd;

    CUITextWnd* m_text;
    bool m_enabledOnFrame;

public:
    CUIButtonHint();

    CUIWindow* Owner() const { return m_ownerWnd; }
    void Discard() { m_ownerWnd = nullptr; }
    void OnRender();
    void Draw_() { m_enabledOnFrame = true; }
    void SetHintText(CUIWindow* w, LPCSTR text);

    pcstr GetDebugType() override { return "CUIButtonHint"; }
};

XRUICORE_API extern CUIButtonHint* g_btnHint;
XRUICORE_API extern CUIButtonHint* g_statHint;
