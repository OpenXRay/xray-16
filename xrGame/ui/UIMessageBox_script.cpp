#include "pch_script.h"
#include "UIMessageBox.h"
#include "UIMessageBoxEx.h"

using namespace luabind;

#pragma optimize("s",on)
void CUIMessageBox::script_register(lua_State *L)
{
	module(L)
	[
		class_<CUIMessageBox,CUIStatic>("CUIMessageBox")
		.def(					constructor<>())
		.def("InitMessageBox",	&CUIMessageBox::InitMessageBox)
		.def("SetText",		&CUIMessageBox::SetText)
		.def("GetHost",		&CUIMessageBox::GetHost)
		.def("GetPassword",	&CUIMessageBox::GetPassword),

		class_<CUIMessageBoxEx, CUIDialogWnd>("CUIMessageBoxEx")
		.def(constructor<>())
		.def("InitMessageBox",	&CUIMessageBoxEx::InitMessageBox)
		.def("SetText",		&CUIMessageBoxEx::SetText)
		.def("GetHost",		&CUIMessageBoxEx::GetHost)
		.def("GetPassword",	&CUIMessageBoxEx::GetPassword)
	];

}