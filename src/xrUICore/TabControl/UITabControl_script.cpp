#include "pch.hpp"
#include "UITabControl.h"
#include "UITabButton.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

SCRIPT_EXPORT(CUITabControl, (CUIWindow), {
    module(luaState)[class_<CUITabControl, CUIWindow>("CUITabControl")
                         .def(constructor<>())
                         .def("AddItem", (bool (CUITabControl::*)(CUITabButton*))(&CUITabControl::AddItem), adopt<2>())
                         .def("AddItem",
                             (bool (CUITabControl::*)(LPCSTR, LPCSTR, Fvector2, Fvector2)) & CUITabControl::AddItem)
                         .def("RemoveAll", &CUITabControl::RemoveAll)
                         .def("GetActiveId", &CUITabControl::GetActiveId_script)
                         .def("GetTabsCount", &CUITabControl::GetTabsCount)
                         .def("SetActiveTab", &CUITabControl::SetActiveTab_script)
                         .def("GetButtonById", &CUITabControl::GetButtonById_script)];
});

SCRIPT_EXPORT(CUITabButton, (CUIButton),
    { module(luaState)[class_<CUITabButton, CUIButton>("CUITabButton").def(constructor<>())]; });
