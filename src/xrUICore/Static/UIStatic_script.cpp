#include "pch.hpp"
#include "UIStatic.h"
#include "UIAnimatedStatic.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUILines, (),
{
    module(luaState)
    [
        class_<CUILines>("CUILines")
            .def("SetFont", &CUILines::SetFont)
            .def("SetText", &CUILines::SetText)
            .def("SetTextST", &CUILines::SetTextST)
            .def("GetText", &CUILines::GetText)
            .def("SetElipsis", &CUILines::SetEllipsis)
            .def("SetTextColor", &CUILines::SetTextColor)
    ];
});

SCRIPT_EXPORT(CUIStatic, (CUIWindow),
{
    module(luaState)
    [
        class_<CUIStatic, CUIWindow>("CUIStatic")
            .def(constructor<>())

            .def("TextControl", &CUIStatic::TextItemControl)

            .def("SetText",   (void (CUIStatic::*)(LPCSTR)) (&CUIStatic::SetText))
            .def("SetTextST", (void (CUIStatic::*)(LPCSTR)) (&CUIStatic::SetTextST))

            .def("GetText", &CUIStatic::GetText)

            .def("SetTextX", &CUIStatic::SetTextX)
            .def("SetTextY", &CUIStatic::SetTextY)
            .def("GetTextX", &CUIStatic::GetTextX)
            .def("GetTextY", &CUIStatic::GetTextY)

            .def("SetColor", &CUIStatic::SetColor)
            .def("GetColor", &CUIStatic::GetColor)

            .def("SetTextColor", &CUIStatic::SetTextColor_script)

            .def("InitTexture", &CUIStatic::InitTexture)
            .def("InitTexture", +[](CUIStatic* self, pcstr texture) { self->InitTexture(texture); })
            .def("InitTextureEx", &CUIStatic::InitTextureEx)
            .def("InitTextureEx", +[](CUIStatic* self, pcstr texture, pcstr shader) { self->InitTextureEx(texture, shader); })

            .def("SetTextureOffset", &CUIStatic::SetTextureOffset)

            .def("SetTextureRect", &CUIStatic::SetTextureRect_script)
            .def("GetTextureRect", &CUIStatic::GetTextureRect_script)

            .def("SetOriginalRect", &CUIStatic::SetTextureRect_script)
            .def("GetOriginalRect", &CUIStatic::GetTextureRect_script)

            .def("SetStretchTexture", &CUIStatic::SetStretchTexture)
            .def("GetStretchTexture", &CUIStatic::GetStretchTexture)

            .def("SetTextAlign", &CUIStatic::SetTextAlign_script)
            .def("GetTextAlign", &CUIStatic::GetTextAlign_script)

            .def("SetHeading", &CUIStatic::SetHeading)
            .def("GetHeading", &CUIStatic::GetHeading)

            //.def("ClipperOn", &CUIStatic::ClipperOn)
            //.def("ClipperOff", (void(CUIStatic::*)(void))&CUIStatic::ClipperOff)

            //.def("GetClipperState", &CUIStatic::GetClipperState)

            .def("SetElipsis", &CUIStatic::SetEllipsis)
    ];
});

SCRIPT_EXPORT(CUITextWnd, (CUIWindow),
{
    module(luaState)
    [
        class_<CUITextWnd, CUIWindow>("CUITextWnd")
            .def(constructor<>())
            .def("AdjustHeightToText", &CUITextWnd::AdjustHeightToText)
            .def("AdjustWidthToText", &CUITextWnd::AdjustWidthToText)
            .def("SetText", &CUITextWnd::SetText)
            .def("SetTextST", &CUITextWnd::SetTextST)
            .def("GetText", &CUITextWnd::GetText)
            .def("SetFont", &CUITextWnd::SetFont)
            .def("GetFont", &CUITextWnd::GetFont)
            .def("SetTextColor", &CUITextWnd::SetTextColor)
            .def("GetTextColor", &CUITextWnd::GetTextColor)
            .def("SetTextComplexMode", &CUITextWnd::SetTextComplexMode)
            .def("SetTextAlignment", &CUITextWnd::SetTextAlignment)
            .def("SetVTextAlignment", &CUITextWnd::SetVTextAlignment)
            .def("SetEllipsis", &CUITextWnd::SetEllipsis)
            .def("SetTextOffset", &CUITextWnd::SetTextOffset)
    ];
});
