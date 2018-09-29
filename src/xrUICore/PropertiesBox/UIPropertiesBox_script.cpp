#include "pch.hpp"
#include "UIPropertiesBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIPropertiesBox, (CUIFrameWindow), {
    module(luaState)[class_<CUIPropertiesBox, CUIFrameWindow>("CUIPropertiesBox")
                         .def(constructor<>())
                         .def("RemoveItem", &CUIPropertiesBox::RemoveItemByTAG)
                         .def("RemoveAll", &CUIPropertiesBox::RemoveAll)
                         .def("Show", (void (CUIPropertiesBox::*)(int, int)) & CUIPropertiesBox::Show)
                         .def("Hide", &CUIPropertiesBox::Hide)
                         //		.def("GetClickedIndex",		&CUIPropertiesBox::GetClickedIndex)
                         .def("AutoUpdateSize", &CUIPropertiesBox::AutoUpdateSize)
                         .def("AddItem", &CUIPropertiesBox::AddItem_script)
    ];
});
