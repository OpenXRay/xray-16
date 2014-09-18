#pragma once

#include "script_callback_ex.h"

class CUIWindow;
struct SCallbackInfo{
	CScriptCallbackEx<void>	m_callback;
	fastdelegate::FastDelegate2<CUIWindow*,void*,void>				m_cpp_callback;
	CUIWindow* 				m_control_ptr;
	shared_str 				m_control_name;
	s16						m_event;
	SCallbackInfo():m_control_ptr(NULL),m_event(-1){};
};

struct event_comparer{
	CUIWindow*			pWnd;
	s16					evt;

	event_comparer(CUIWindow* w, s16 e):pWnd(w),evt(e){}
	bool operator ()(SCallbackInfo* i);
};