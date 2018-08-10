#include "pch.hpp"
#include "UIWndCallback.h"
#include "Windows/UIWindow.h"
#include "Common/object_broker.h"
#include "callback_info.h"

bool event_comparer::operator()(SCallbackInfo* i)
{
    if (i->m_event == evt)
    {
        return (i->m_control_ptr) ? (i->m_control_ptr == pWnd) : (i->m_control_name == pWnd->WindowName());
    }
    else
        return false;
}

CUIWndCallback::~CUIWndCallback() { delete_data(m_callbacks); }
void CUIWndCallback::Register(CUIWindow* pChild) { pChild->SetMessageTarget(smart_cast<CUIWindow*>(this)); }
void CUIWndCallback::OnEvent(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (!pWnd)
        return;
    event_comparer ec(pWnd, msg);

    CALLBACK_IT it = std::find_if(m_callbacks.begin(), m_callbacks.end(), ec);
    if (it == m_callbacks.end())
        return;

    (*it)->m_callback();

    if ((*it)->m_cpp_callback)
        (*it)->m_cpp_callback(pWnd, pData);
}

SCallbackInfo* CUIWndCallback::NewCallback()
{
    m_callbacks.push_back(new SCallbackInfo());
    return m_callbacks.back();
}

void CUIWndCallback::AddCallback(CUIWindow* pWnd, s16 evt, const void_function& f)
{
    SCallbackInfo* c = NewCallback();
    c->m_cpp_callback = f;
    c->m_control_ptr = pWnd;
    c->m_control_name = "noname";
    c->m_event = evt;
}

void CUIWndCallback::AddCallbackStr(const shared_str& control_id, s16 evt, const void_function& f)
{
    SCallbackInfo* c = NewCallback();
    c->m_cpp_callback = f;
    c->m_control_ptr = NULL;
    c->m_control_name = control_id;
    c->m_event = evt;
}
