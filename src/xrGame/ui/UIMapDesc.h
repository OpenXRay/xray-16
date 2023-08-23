#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUIScrollView;
class CUI3tButton;
class CUIMapInfo;
class CUIStatsPlayerList;

class CUIMapDesc final : public CUIDialogWnd
{
public:
    CUIMapDesc();
    ~CUIMapDesc() override;

    void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0) override;
    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;

    pcstr GetDebugType() override { return "CUIMapDesc"; }

private:
    void Init();

    CUIStatic* m_pCaption;
    CUIStatic* m_pBackground;
    CUIStatic* m_pFrame[3];
    CUIScrollView* m_pTextDesc;
    CUIStatic* m_pImage;
    CUI3tButton* m_pBtnSpectator;
    CUI3tButton* m_pBtnNext;
    CUIMapInfo* m_pMapInfo;
};
