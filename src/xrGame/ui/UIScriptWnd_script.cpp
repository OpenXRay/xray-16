#include "pch_script.h"
#include "UIScriptWnd.h"
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
    ];
});
// clang-format on
