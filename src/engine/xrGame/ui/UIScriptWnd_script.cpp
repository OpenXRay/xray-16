#include "pch_script.h"

//UI-controls
#include "UIScriptWnd.h"
#include "UIButton.h"
#include "UIMessageBox.h"
#include "UIPropertiesBox.h"
#include "UICheckButton.h"
#include "UIRadioButton.h"
#include "UIStatic.h"
#include "UIEditBox.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "UIProgressBar.h"
#include "UITabControl.h"

#include "uiscriptwnd_script.h"

using namespace luabind;

extern export_class &script_register_ui_window1(export_class &);
extern export_class &script_register_ui_window2(export_class &);

#pragma optimize("s",on)
void CUIDialogWndEx::script_register(lua_State *L)
{
	export_class				instance("CUIScriptWnd");

	module(L)
	[
		script_register_ui_window2(
			script_register_ui_window1(
				instance
			)
		)
		.def("Load",			&BaseType::Load)
	];
}

export_class &script_register_ui_window1(export_class &instance)
{
	instance
		.def(					constructor<>())

		.def("AddCallback",		(void(BaseType::*)(LPCSTR, s16, const luabind::functor<void>&, const luabind::object&))&BaseType::AddCallback)

		.def("Register",		(void (BaseType::*)(CUIWindow*,LPCSTR))&BaseType::Register)

	;return	(instance);
}
