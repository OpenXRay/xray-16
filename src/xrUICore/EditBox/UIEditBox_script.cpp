#include "pch.hpp"
#include "UIEditBox.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUIEditBox, (CUIWindow),
{
    using namespace luabind;

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
            .def("InitTexture", +[](CUIEditBox* self, pcstr texture) { self->InitTexture(texture); })
    ];
});
