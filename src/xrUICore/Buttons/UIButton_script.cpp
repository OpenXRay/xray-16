#include "pch.hpp"
#include "UIButton.h"
#include "UI3tButton.h"
#include "UICheckButton.h"
#include "SpinBox/UISpinNum.h"
#include "SpinBox/UISpinText.h"
#include "TrackBar/UITrackBar.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUIButton, (CUIStatic, CUIWindow),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIButton, CUIStatic>("CUIButton")
            .def("Init", +[](CUIButton* self, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
            })
            .def("Init", +[](CUIButton* self, cpcstr texture, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
                self->InitTexture(texture);
            })
            .def(constructor<>()),

        class_<CUI3tButton, CUIButton>("CUI3tButton")
            .def(constructor<>()),

        class_<CUICheckButton, CUI3tButton>("CUICheckButton")
            .def(constructor<>())
            .def("GetCheck", &CUICheckButton::GetCheck)
            .def("SetCheck", &CUICheckButton::SetCheck)
            .def("SetDependControl", &CUICheckButton::SetDependControl),

        class_<CUICustomSpin, CUIWindow>("CUICustomSpin")
            .def("Init", +[](CUICustomSpin* self, float x, float y, float width, float height)
            {
                const Fvector2 pos { x, y };
                const Fvector2 size { width, height };

                self->InitSpin(pos, size);
            })
            .def("GetText", &CUICustomSpin::GetText),

        class_<CUISpinNum, CUICustomSpin>("CUISpinNum")
            .def(constructor<>()),

        class_<CUISpinFlt, CUICustomSpin>("CUISpinFlt")
            .def(constructor<>()),

        class_<CUISpinText, CUICustomSpin>("CUISpinText")
            .def(constructor<>()),

        class_<CUITrackBar, CUIWindow>("CUITrackBar")
            .def(constructor<>())
            .def("GetCheck", &CUITrackBar::GetCheck)
            .def("SetCheck", &CUITrackBar::SetCheck)
            .def("GetIValue", &CUITrackBar::GetIValue)
            .def("GetFValue", &CUITrackBar::GetFValue)
            .def("SetOptIBounds", &CUITrackBar::SetOptIBounds)
            .def("SetOptFBounds", &CUITrackBar::SetOptFBounds)
            .def("SetCurrentValue", &CUITrackBar::SetCurrentOptValue)];
});
