#include "pch.hpp"
#include "UITabControl.h"
#include "UITabButton.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

SCRIPT_EXPORT(CUITabControl, (CUIWindow),
{
    module(luaState)
    [
        class_<CUITabControl, CUIWindow>("CUITabControl")
            .def(constructor<>())
            .def("AddItem", (bool (CUITabControl::*)(CUITabButton*))(&CUITabControl::AddItem), adopt<2>())
            .def("AddItem", (bool (CUITabControl::*)(LPCSTR, LPCSTR, Fvector2, Fvector2)) & CUITabControl::AddItem)
            .def("RemoveItem", &CUITabControl::RemoveItemByIndex)
            .def("RemoveItem", &CUITabControl::RemoveItemById_script)
            .def("RemoveAll", &CUITabControl::RemoveAll)
            .def("GetActiveId", &CUITabControl::GetActiveId_script)
            .def("GetActiveIndex", &CUITabControl::GetActiveIndex)
            .def("GetTabsCount", &CUITabControl::GetTabsCount)
            .def("SetActiveTab", &CUITabControl::SetActiveTab_script)
            .def("SetActiveTab", &CUITabControl::SetActiveTabByIndex)
            .def("SetNewActiveTab", &CUITabControl::SetActiveTabByIndex)
            .def("GetButtonById", &CUITabControl::GetButtonById_script)
            .def("GetButtonByIndex", &CUITabControl::GetButtonByIndex)
    ];
});

SCRIPT_EXPORT(CUITabButton, (CUIButton),
    { module(luaState)[class_<CUITabButton, CUIButton>("CUITabButton").def(constructor<>())]; });
