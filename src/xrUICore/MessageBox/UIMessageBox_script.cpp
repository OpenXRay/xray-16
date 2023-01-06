#include "pch.hpp"
#include "UIMessageBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUIMessageBox, (CUIStatic),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIMessageBox, CUIStatic>("CUIMessageBox")
            .def(constructor<>())
            .def("Init", &CUIMessageBox::InitMessageBox)
            .def("InitMessageBox", &CUIMessageBox::InitMessageBox)
            .def("SetText", &CUIMessageBox::SetText)
            .def("GetHost", &CUIMessageBox::GetHost)
            .def("GetPassword", &CUIMessageBox::GetPassword)
    ];
});
