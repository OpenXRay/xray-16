#include "StdAfx.h"
#include "UIVoteStatusWnd.h"
#include "UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"

void UIVoteStatusWnd::InitFromXML(CUIXml& xml_doc)
{
    m_str_message = xr_new<CUIStatic>("Message");
    m_str_message->SetAutoDelete(true);
    AttachChild(m_str_message);
    m_hint = xr_new<CUIStatic>("Hint");
    m_hint->SetAutoDelete(true);
    AttachChild(m_hint);
    m_time_message = xr_new<CUIStatic>("Time message");
    m_time_message->SetAutoDelete(true);
    AttachChild(m_time_message);

    CUIXmlInit::InitFrameWindow(xml_doc, "vote_wnd", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "vote_wnd:static_str_message", 0, m_str_message);
    CUIXmlInit::InitStatic(xml_doc, "vote_wnd:static_hint", 0, m_hint);
    CUIXmlInit::InitStatic(xml_doc, "vote_wnd:static_time_message", 0, m_time_message);
}

void UIVoteStatusWnd::SetVoteTimeResultMsg(LPCSTR s) { m_time_message->SetText(s); }
void UIVoteStatusWnd::SetVoteMsg(LPCSTR s) { m_str_message->SetText(s); }
