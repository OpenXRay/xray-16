#include "pch_script.h"

// UI-controls
#include "UIScriptWnd.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/Functor.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIDialogWndEx, (CUIDialogWnd, IFactoryObject), {
    module(luaState)[class_<CUIDialogWndEx, bases<CUIDialogWnd, IFactoryObject>>("CUIScriptWnd")
        .def("OnKeyboard", &CUIDialogWndEx::OnKeyboardAction)
        .def("Update", &CUIDialogWndEx::Update)
        .def("Dispatch", &CUIDialogWndEx::Dispatch)
        .def("AddCallback", (void (CUIDialogWndEx::*)(LPCSTR, s16, const functor<void>&, const object&)) &
            CUIDialogWndEx::AddCallback)
        .def("Register", (void (CUIDialogWndEx::*)(CUIWindow*, LPCSTR)) &CUIDialogWndEx::Register)
        .def("Load", &CUIDialogWndEx::Load)
    ];
});

