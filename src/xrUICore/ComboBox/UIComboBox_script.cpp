// File:        UIComboBox_script.cpp
// Description: exports CUIComobBox to LUA environment
// Created:     11.12.2004
// Author:      Serhiy O. Vynnychenko
// Mail:        narrator@gsc-game.kiev.ua
//
// Copyright 2004 GSC Game World
//

#include "pch.hpp"
#include "UIComboBox.h"
#include "ListBox/UIListBoxItem.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUIComboBox, (CUIWindow),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIComboBox, CUIWindow>("CUIComboBox")
            .def(constructor<>())
            .def("Init", +[](CUIComboBox* self, float x, float y, float width)
            {
                const Fvector2 pos { x, y };
                self->InitComboBox(pos, width);
            })
            .def("Init", +[](CUIComboBox* self, float x, float y, float width, float /*height*/)
            {
                const Fvector2 pos { x, y };
                self->InitComboBox(pos, width);
            })
            .def("SetVertScroll", &CUIComboBox::SetVertScroll)
            .def("SetListLength", &CUIComboBox::SetListLength)
            .def("CurrentID", &CUIComboBox::CurrentID)
            .def("SetCurrentID", &CUIComboBox::SetItemIDX)
            .def("disable_id", &CUIComboBox::disable_id)
            .def("enable_id", &CUIComboBox::enable_id)
            .def("AddItem", &CUIComboBox::AddItem_)
            .def("GetText", &CUIComboBox::GetText)
            .def("GetTextOf", &CUIComboBox::GetTextOf)
            .def("SetText", &CUIComboBox::SetText)
            .def("ClearList", &CUIComboBox::ClearList)
            .def("SetCurrentValue", &CUIComboBox::SetCurrentOptValue)
            .def("SetCurrentOptValue", &CUIComboBox::SetCurrentOptValue)
            .def("SetCurrentIdx", &CUIComboBox::SetSelectedIDX)
            .def("GetCurrentIdx", &CUIComboBox::GetSelectedIDX)
    ];
});
