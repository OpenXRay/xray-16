#include "pch_script.h"
#include "UIMessageBox.h"
#include "UIMessageBoxEx.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIMessageBox, (CUIStatic),
{
    module(luaState)
    [
        class_<CUIMessageBox,CUIStatic>("CUIMessageBox")
        .def(constructor<>())
        .def("InitMessageBox",	&CUIMessageBox::InitMessageBox)
        .def("SetText",		&CUIMessageBox::SetText)
        .def("GetHost",		&CUIMessageBox::GetHost)
        .def("GetPassword",	&CUIMessageBox::GetPassword)
    ];
});

SCRIPT_EXPORT(CUIMessageBoxEx, (CUIDialogWnd),
{
    module(luaState)
    [
		class_<CUIMessageBoxEx, CUIDialogWnd>("CUIMessageBoxEx")
		.def(constructor<>())
		.def("InitMessageBox",	&CUIMessageBoxEx::InitMessageBox)
		.def("SetText",		&CUIMessageBoxEx::SetText)
		.def("GetHost",		&CUIMessageBoxEx::GetHost)
		.def("GetPassword",	&CUIMessageBoxEx::GetPassword)
	];
});
