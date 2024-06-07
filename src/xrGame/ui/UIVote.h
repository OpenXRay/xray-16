#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUI3tButton;
class CUIListBox;
class CUIFrameWindow;

class CUIVote final : public CUIDialogWnd
{
public:
    CUIVote();

    void Update() override;
    void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0) override;
    void OnBtnYes();
    void OnBtnNo();
    void OnBtnCancel();
    void SetVoting(LPCSTR txt);

    pcstr GetDebugType() override { return "CUIVote"; }

protected:
    CUIStatic* msg;
    CUIListBox* list[3];

    CUI3tButton* btn_yes;
    CUI3tButton* btn_no;
    CUI3tButton* btn_cancel;

    u32 m_prev_upd_time{};
};
