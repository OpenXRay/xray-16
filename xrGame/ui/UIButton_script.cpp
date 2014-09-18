#include "pch_script.h"
#include "UIButton.h"
#include "UI3tButton.h"
#include "UICheckButton.h"
#include "UIRadioButton.h"
#include "UISpinNum.h"
#include "UISpinText.h"
#include "UITrackBar.h"

using namespace luabind;

#pragma optimize("s",on)
void CUIButton::script_register(lua_State *L)
{
	module(L)
	[
		class_<CUIButton, CUIStatic>("CUIButton")
		.def(							constructor<>())
		,

		class_<CUI3tButton, CUIButton>("CUI3tButton")
		.def(							constructor<>())
		,


		class_<CUICheckButton, CUI3tButton>("CUICheckButton")
		.def(							constructor<>())
		.def("GetCheck",				&CUICheckButton::GetCheck)
		.def("SetCheck",				&CUICheckButton::SetCheck)
		.def("SetDependControl",		&CUICheckButton::SetDependControl),

		class_<CUICustomSpin, CUIWindow>("CUICustomSpin")
		.def("GetText",				&CUICustomSpin::GetText),

		class_<CUISpinNum, CUICustomSpin>("CUISpinNum")
		.def(							constructor<>()),

		class_<CUISpinFlt, CUICustomSpin>("CUISpinFlt")
		.def(							constructor<>()),

		class_<CUISpinText, CUICustomSpin>("CUISpinText")
		.def(							constructor<>()),

		class_<CUITrackBar, CUIWindow>("CUITrackBar")
		.def(							constructor<>())
		.def("GetCheck",				&CUITrackBar::GetCheck)
		.def("SetCheck",				&CUITrackBar::SetCheck)
		.def("GetIValue",				&CUITrackBar::GetIValue)
		.def("GetFValue",				&CUITrackBar::GetFValue)
		.def("SetOptIBounds",			&CUITrackBar::SetOptIBounds)
		.def("SetOptFBounds",			&CUITrackBar::SetOptFBounds)
		.def("SetCurrentValue",			&CUITrackBar::SetCurrentOptValue)
	];
}