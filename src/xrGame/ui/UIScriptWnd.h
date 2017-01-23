#pragma once

#include "UIDialogWnd.h"
#include "xrScriptEngine/script_space_forward.hpp"

struct SCallbackInfo;

class CUIDialogWndEx :public CUIDialogWnd, public FactoryObjectBase
{
typedef CUIDialogWnd				inherited;
typedef xr_vector<SCallbackInfo*>	CALLBACKS;
typedef CALLBACKS::iterator			CALLBACK_IT;

private:
			CALLBACKS			m_callbacks;
	virtual void				SendMessage			(CUIWindow* pWnd, s16 msg, void* pData = NULL);
			SCallbackInfo*		NewCallback			();
public:
			bool				Load				(LPCSTR xml_name);
			void				Register			(CUIWindow* pChild);
			void				Register			(CUIWindow* pChild, LPCSTR name);
								CUIDialogWndEx		();
	virtual						~CUIDialogWndEx		();
			void				AddCallback			(LPCSTR control_id, s16 event, const luabind::functor<void> &functor, const luabind::object &object);
	virtual void				Update				();
	virtual bool				OnKeyboardAction			(int dik, EUIMessages keyboard_action);
	virtual bool				Dispatch			(int cmd, int param)				{return true;}
/*
template<typename T>
IC	T*	GetControl(LPCSTR name);
*/
};
