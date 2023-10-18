#pragma once
#include "xrUICore/Windows/UIFrameWindow.h"

class CUIXml;
class CUITextWnd;

class UIVoteStatusWnd final : public CUIFrameWindow
{
    CUITextWnd* m_str_message{};
    CUITextWnd* m_hint{};
    CUITextWnd* m_time_message{};

public:
    UIVoteStatusWnd() : CUIFrameWindow(UIVoteStatusWnd::GetDebugType()) {}

    void InitFromXML(CUIXml& xml_doc);
    void SetVoteTimeResultMsg(LPCSTR s);
    void SetVoteMsg(LPCSTR s);

    pcstr GetDebugType() override { return "UIVoteStatusWnd"; }
};
