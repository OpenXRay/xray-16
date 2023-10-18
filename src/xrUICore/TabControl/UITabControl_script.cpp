#include "pch.hpp"
#include "UITabControl.h"
#include "UITabButton.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUITabControl, (CUIWindow),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUITabControl, CUIWindow>("CUITabControl")
            .def(constructor<>())
            .def("AddItem", (bool (CUITabControl::*)(CUITabButton*))(&CUITabControl::AddItem), adopt<2>())
            .def("AddItem", (bool (CUITabControl::*)(pcstr, pcstr, Fvector2, Fvector2)) &CUITabControl::AddItem)
            .def("AddItem", +[](CUITabControl* self, pcstr pItemName, pcstr pTexName, float x, float y, float width, float height)
            {
                self->AddItem(pItemName, pTexName, { x, y }, { width, height });
            })
            .def("RemoveItem", &CUITabControl::RemoveItemByIndex)
            .def("RemoveItemById", &CUITabControl::RemoveItemById_script)
            .def("RemoveAll", &CUITabControl::RemoveAll)
            .def("GetActiveId", &CUITabControl::GetActiveId_script)
            .def("GetActiveIndex", &CUITabControl::GetActiveIndex)
            .def("GetTabsCount", &CUITabControl::GetTabsCount)
            .def("SetActiveTab", &CUITabControl::SetActiveTab_script)
            .def("SetNewActiveTab", &CUITabControl::SetActiveTabByIndex)
            .def("GetButtonById", &CUITabControl::GetButtonById_script)
            .def("GetButtonByIndex", &CUITabControl::GetButtonByIndex)
    ];
});

SCRIPT_EXPORT(CUITabButton, (CUIButton),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUITabButton, CUIButton>("CUITabButton")
            .def(constructor<>())
    ];
});
