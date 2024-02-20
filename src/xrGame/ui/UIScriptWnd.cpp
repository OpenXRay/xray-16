#include "pch_script.h"
#include "UIScriptWnd.h"
#include "Common/object_broker.h"
#include "xrUICore/Callbacks/callback_info.h"

CUIDialogWndEx::CUIDialogWndEx() : CUIDialogWnd("CUIDialogWndEx") {}
CUIDialogWndEx::~CUIDialogWndEx() { delete_data(m_callbacks); }

void CUIDialogWndEx::Register(CUIWindow* pChild) { pChild->SetMessageTarget(this); }
void CUIDialogWndEx::Register(CUIWindow* pChild, pcstr name)
{
    pChild->SetWindowName(name);
    pChild->SetMessageTarget(this);
}

void CUIDialogWndEx::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    const auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(), event_comparer{ pWnd, msg });
    if (it == m_callbacks.end())
        return inherited::SendMessage(pWnd, msg, pData);

    (*it)->m_callback();

    //	if ( (*it)->m_cpp_callback )
    //		(*it)->m_cpp_callback(pData);
}

bool CUIDialogWndEx::Load(pcstr /*xml_name*/) { return true; }

SCallbackInfo* CUIDialogWndEx::NewCallback()
{
    m_callbacks.push_back(xr_new<SCallbackInfo>());
    return m_callbacks.back();
}

void CUIDialogWndEx::AddCallback(LPCSTR control_id, s16 evt, const luabind::functor<void> &lua_function)
{
    SCallbackInfo* c = NewCallback ();
    c->m_callback.set(lua_function);
    c->m_control_name = control_id;
    c->m_event = evt;
}

void CUIDialogWndEx::AddCallback(pcstr control_id, s16 evt, const luabind::functor<void>& functor, const luabind::object& object)
{
    SCallbackInfo* c = NewCallback();
    c->m_callback.set(functor, object);
    c->m_control_name = control_id;
    c->m_event = evt;
}
