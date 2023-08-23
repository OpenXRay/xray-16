#include "pch.hpp"
#include "UIStatic.h"
#include "UIAnimatedStatic.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUILines, (),
{
    using namespace luabind;

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

// We don't change game assets.
// This class allowes original game scripts to not specify the window name.
class CUIStaticScript final : public CUIStatic
{
public:
    CUIStaticScript() : CUIStatic("CUIStaticScript") {}
    pcstr GetDebugType() override { return "CUIStaticScript"; }
};

SCRIPT_EXPORT(CUIStatic, (CUIWindow),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIStatic, CUIWindow>("CUIStaticBase")
            .def(constructor<pcstr>())

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

            .def("Init", +[](CUIStatic* self, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
            })
            .def("Init", +[](CUIStatic* self, cpcstr texture, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
                self->InitTexture(texture);
            })

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

            .def("SetElipsis", &CUIStatic::SetEllipsis),

        class_<CUIStaticScript, CUIStatic>("CUIStatic")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CUITextWnd, (CUIWindow),
{
    using namespace luabind;

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
