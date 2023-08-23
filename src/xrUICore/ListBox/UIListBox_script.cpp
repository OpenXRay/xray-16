#include "pch.hpp"
#include "UIListBox.h"
#include "UIListBoxItem.h"
#include "UIListBoxItemMsgChain.h"
#include "SpinBox/UISpinText.h"
#include "ComboBox/UIComboBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

struct CUIListBoxItemWrapper : public CUIListBoxItem, public luabind::wrap_base
{
    CUIListBoxItemWrapper(float h) : CUIListBoxItem(h) {}
    pcstr GetDebugType() override { return "CUIListBoxItemScript"; }
};

struct CUIListBoxItemMsgChainWrapper : public CUIListBoxItemMsgChain, public luabind::wrap_base
{
    CUIListBoxItemMsgChainWrapper(float h) : CUIListBoxItemMsgChain(h) {}
    pcstr GetDebugType() override { return "CUIListBoxItemMsgChainScript"; }
};

// clang-format off
SCRIPT_EXPORT(CUIListBox, (CUIScrollView),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUIListBox, CUIScrollView>("CUIListBox")
            .def(constructor<>())
            .def("ShowSelectedItem", &CUIListBox::Show)
            .def("RemoveAll", &CUIListBox::Clear)
            .def("GetSize", &CUIListBox::GetSize)
            .def("GetSelectedItem", &CUIListBox::GetSelectedItem)
            .def("GetSelectedIndex", &CUIListBox::GetSelectedIDX)
            .def("SetSelectedIndex", &CUIListBox::SetSelectedIDX)
            .def("SetItemHeight", &CUIListBox::SetItemHeight)
            .def("GetItemHeight", &CUIListBox::GetItemHeight)
            .def("GetItemByIndex", &CUIListBox::GetItemByIDX)
            .def("GetItem", &CUIListBox::GetItem)
            .def("RemoveItem", &CUIListBox::RemoveWindow)
            .def("AddTextItem", &CUIListBox::AddTextItem)
            .def("AddExistingItem", &CUIListBox::AddExistingItem, adopt<2>())
    ];
});

SCRIPT_EXPORT(CUIListBoxItem, (CUIFrameLineWnd),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIListBoxItem, CUIFrameLineWnd, default_holder, CUIListBoxItemWrapper>("CUIListBoxItem")
            .def(constructor<float>())
            .def("GetTextItem", &CUIListBoxItem::GetTextItem)
            .def("AddTextField", &CUIListBoxItem::AddTextField)
            .def("AddIconField", &CUIListBoxItem::AddIconField)
            .def("SetTextColor", &CUIListBoxItem::SetTextColor)
    ];
});

SCRIPT_EXPORT(CUIListBoxItemMsgChain, (CUIListBoxItem),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIListBoxItemMsgChain, CUIListBoxItem, default_holder, CUIListBoxItemMsgChainWrapper>("CUIListBoxItemMsgChain")
            .def(constructor<float>())
    ];
});
// clang-format on
