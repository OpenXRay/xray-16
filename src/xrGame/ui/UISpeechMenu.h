#pragma once
#include "UIDialogWnd.h"

class CUIScrollView;

class CUISpeechMenu final : public CUIDialogWnd
{
public:
    CUISpeechMenu(LPCSTR section_name);

    void InitList(LPCSTR section_name);
    bool NeedCursor() const override { return false; }
    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;
    bool StopAnyMove() override { return false; }
    pcstr GetDebugType() override { return "CUISpeechMenu"; }

private:
    CUIScrollView* m_pList;
    u32 m_text_color;
    CGameFont* m_pFont;
};
