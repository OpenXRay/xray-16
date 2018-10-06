#include "pch.hpp"
#include "UIEditBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIEditBox, (CUIWindow), {
    module(luaState)
    [
        class_<CUICustomEdit, CUIWindow>("CUICustomEdit")
            .def("SetText", &CUICustomEdit::SetText)
            .def("GetText", &CUICustomEdit::GetText)
            .def("CaptureFocus", &CUICustomEdit::CaptureFocus)
            .def("SetNextFocusCapturer", &CUICustomEdit::SetNextFocusCapturer),

        class_<CUIEditBox, CUICustomEdit>("CUIEditBox")
            .def(constructor<>())
            .def("InitTexture", &CUIEditBox::InitTexture)
    ];
});
