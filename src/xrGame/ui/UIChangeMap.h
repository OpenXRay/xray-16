#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUI3tButton;
class CUIFrameWindow;
class CUIListBox;
class CUIXml;

class CUIChangeMap final : public CUIDialogWnd
{
public:
    CUIChangeMap();
    void InitChangeMap(CUIXml& xml_doc);

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0);

    void OnBtnOk();
    void OnBtnCancel();
    void OnItemSelect();

    pcstr GetDebugType() override { return "CUIChangeMap"; }

protected:
    void FillUpList();

    CUIStatic* map_pic{};
    CUIStatic* map_version{};
    CUIListBox* lst{};

    CUI3tButton* btn_ok{};
    CUI3tButton* btn_cancel{};

    u32 m_prev_upd_time{};
};
