#include "pch_script.h"
#include "UIStatic.h"
#include "UIAnimatedStatic.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUILines, (), {
    module(luaState)[class_<CUILines>("CUILines")
                         .def("SetFont", &CUILines::SetFont)
                         .def("SetText", &CUILines::SetText)
                         .def("SetTextST", &CUILines::SetTextST)
                         .def("GetText", &CUILines::GetText)
                         .def("SetElipsis", &CUILines::SetEllipsis)
                         .def("SetTextColor", &CUILines::SetTextColor)];
});

SCRIPT_EXPORT(CUIStatic, (CUIWindow), {
    module(luaState)[class_<CUIStatic, CUIWindow>("CUIStatic")
                         .def(constructor<>())
                         .def("TextControl", &CUIStatic::TextItemControl)
                         .def("InitTexture", &CUIStatic::InitTexture)
                         .def("InitTextureEx", &CUIStatic::InitTextureEx)
                         .def("SetTextureRect", &CUIStatic::SetTextureRect_script)
                         .def("SetStretchTexture", &CUIStatic::SetStretchTexture)
                         .def("GetTextureRect", &CUIStatic::GetTextureRect_script)];
});

SCRIPT_EXPORT(CUITextWnd, (CUIWindow), {
    module(luaState)[class_<CUITextWnd, CUIWindow>("CUITextWnd")
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
        //		.def("",					&CUITextWnd::)
    ];
});

SCRIPT_EXPORT(CUISleepStatic, (CUIStatic),
    { module(luaState)[class_<CUISleepStatic, CUIStatic>("CUISleepStatic").def(constructor<>())]; });
