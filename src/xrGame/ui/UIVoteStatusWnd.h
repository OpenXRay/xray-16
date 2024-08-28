#pragma once
#include "xrUICore/Windows/UIFrameWindow.h"

class CUIXml;
class CUIStatic;

class UIVoteStatusWnd final : public CUIFrameWindow
{
    CUIStatic* m_str_message{};
    CUIStatic* m_hint{};
    CUIStatic* m_time_message{};

public:
    UIVoteStatusWnd() : CUIFrameWindow(UIVoteStatusWnd::GetDebugType()) {}

    void InitFromXML(CUIXml& xml_doc);
    void SetVoteTimeResultMsg(LPCSTR s);
    void SetVoteMsg(LPCSTR s);

    pcstr GetDebugType() override { return "UIVoteStatusWnd"; }
};
