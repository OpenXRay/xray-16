#include "pch.hpp"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "UIListWnd.h"
#include "UIListItemEx.h"

bool CUIListWnd::AddItem_script(CUIListItem* item)
{
    return AddItem(item, -1);
}

struct CUIListItemWrapper final : public CUIListItem, public luabind::wrap_base
{
    pcstr GetDebugType() override { return "CUIListItemScript"; }
};

struct CUIListItemExWrapper final : public CUIListItemEx, public luabind::wrap_base
{
    pcstr GetDebugType() override { return "CUIListItemExScript"; }
};

// clang-format off
#pragma optimize("s", on)
SCRIPT_EXPORT(CUIListWnd, (CUIWindow),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUIListWnd, CUIWindow>("CUIListWnd")
            .def(constructor<>())
            //.def("AddText", &CUIListWnd::AddText_script)
            .def("AddItem", &CUIListWnd::AddItem_script, adopt<2>())
            .def("RemoveItem", &CUIListWnd::RemoveItem)
            .def("RemoveAll", &CUIListWnd::RemoveAll)
            .def("EnableScrollBar", &CUIListWnd::EnableScrollBar)
            .def("IsScrollBarEnabled", &CUIListWnd::IsScrollBarEnabled)
            .def("ScrollToBegin", &CUIListWnd::ScrollToBegin)
            .def("ScrollToEnd", &CUIListWnd::ScrollToEnd)
            .def("SetItemHeight", &CUIListWnd::SetItemHeight)
            .def("GetItem", &CUIListWnd::GetItem)
            .def("GetItemPos", &CUIListWnd::GetItemPos)
            .def("GetSize", &CUIListWnd::GetItemsCount)
            .def("ScrollToBegin", &CUIListWnd::ScrollToBegin)
            .def("ScrollToEnd", &CUIListWnd::ScrollToEnd)
            .def("ScrollToPos", &CUIListWnd::ScrollToPos)
            .def("SetWidth", &CUIListWnd::SetWidth)
            .def("SetTextColor", &CUIListWnd::SetTextColor)
            .def("ActivateList", &CUIListWnd::ActivateList)
            .def("IsListActive", &CUIListWnd::IsListActive)
            .def("SetVertFlip", &CUIListWnd::SetVertFlip)
            .def("GetVertFlip", &CUIListWnd::GetVertFlip)
            .def("SetFocusedItem", &CUIListWnd::SetFocusedItem)
            .def("GetFocusedItem", &CUIListWnd::GetFocusedItem)
            .def("ShowSelectedItem", &CUIListWnd::ShowSelectedItem)

            .def("GetSelectedItem", &CUIListWnd::GetSelectedItem)
            .def("ResetFocusCapture", &CUIListWnd::ResetFocusCapture)
    ];
});

SCRIPT_EXPORT(CUIListItem, (CUIButton),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIListItem, CUIButton, default_holder, CUIListItemWrapper>("CUIListItem")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CUIListItemEx, (CUIListItem),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIListItemEx, CUIListItem, default_holder, CUIListItemExWrapper>("CUIListItemEx")
            .def(constructor<>())
            .def("SetSelectionColor", &CUIListItemEx::SetSelectionColor)
    ];
});
// clang-format on
