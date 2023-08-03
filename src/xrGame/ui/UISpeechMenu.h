#pragma once
#include "UIDialogWnd.h"

class CUIScrollView;

class CUISpeechMenu final : public CUIDialogWnd
{
public:
    CUISpeechMenu(LPCSTR section_name);
    void InitList(LPCSTR section_name);
    virtual bool NeedCursor() const { return false; }
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool StopAnyMove() { return false; }
private:
    CUIScrollView* m_pList;
    u32 m_text_color;
    CGameFont* m_pFont;
};
