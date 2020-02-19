#pragma once
#include "xrUICore/Windows/UIFrameWindow.h"

class CUIXml;
class CUITextWnd;

class UIVoteStatusWnd : public CUIFrameWindow
{
    CUITextWnd* m_str_message;
    CUITextWnd* m_hint;
    CUITextWnd* m_time_message;

public:
    void InitFromXML(CUIXml& xml_doc);
    void SetVoteTimeResultMsg(const char* s);
    void SetVoteMsg(const char* s);
};
