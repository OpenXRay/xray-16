#include "pch.hpp"
#include "UIMessageBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIMessageBox, (CUIStatic), {
    module(luaState)[class_<CUIMessageBox, CUIStatic>("CUIMessageBox")
                         .def(constructor<>())
                         .def("InitMessageBox", &CUIMessageBox::InitMessageBox)
                         .def("SetText", &CUIMessageBox::SetText)
                         .def("GetHost", &CUIMessageBox::GetHost)
                         .def("GetPassword", &CUIMessageBox::GetPassword)];
});
