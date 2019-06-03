#include "pch_script.h"
#include "UIScriptWnd.h"
#include "xrUICore/TabControl/UITabControl.h" // Don't remove
#include "uiscriptwnd_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/Functor.hpp"

using namespace luabind;

// clang-format off
SCRIPT_EXPORT(CUIDialogWndEx, (CUIDialogWnd, IFactoryObject),
{
    module(luaState)
    [
        luabind::class_<CUIDialogWndEx, luabind::bases<CUIDialogWnd, IFactoryObject>, luabind::default_holder, WrapType>("CUIScriptWnd")
            .def(constructor<>())
            .def("AddCallback", (void (BaseType::*)(LPCSTR, s16, const luabind::functor<void>&, const luabind::object&)) &
                BaseType::AddCallback)
            .def("Register", (void (BaseType::*)(CUIWindow*, LPCSTR)) & BaseType::Register)
            .def("OnKeyboard", &BaseType::OnKeyboardAction, &WrapType::OnKeyboard_static)
            .def("Update", &BaseType::Update, &WrapType::Update_static)
            .def("Dispatch", &BaseType::Dispatch, &WrapType::Dispatch_static)
            .def("Load", &BaseType::Load)

            .def("GetStatic", (CUIStatic* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIStatic>)
            .def("GetEditBox", (CUIEditBox* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIEditBox>)
            .def("GetDialogWnd", (CUIDialogWnd* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIDialogWnd>)
            .def("GetFrameWindow", (CUIFrameWindow* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIFrameWindow>)
            .def("GetFrameLineWnd", (CUIFrameLineWnd* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIFrameLineWnd>)
            .def("GetProgressBar", (CUIProgressBar* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIProgressBar>)
            .def("GetTabControl", (CUITabControl* (BaseType::*)(pcstr)) &BaseType::GetControl<CUITabControl>)
            // XXX: ListWnd and ListBox has the same functionality but different function prototypes
            // We should not use ListBox for CS and SOC, we should return ListWnd class
            //.def("GetListWnd", (CUIListBox* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIListBox>)
    ];
});
// clang-format on
