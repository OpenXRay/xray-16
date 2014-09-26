#include "pch_script.h"
#include "UIPropertiesBox.h"

using namespace luabind;

#pragma optimize("s",on)
void CUIPropertiesBox::script_register(lua_State *L)
{
	module(L)
	[
		class_<CUIPropertiesBox,CUIFrameWindow>("CUIPropertiesBox")
		.def(					constructor<>())
//		.def("AddItem",					&CUIPropertiesBox::AddItem)
		.def("RemoveItem",			&CUIPropertiesBox::RemoveItemByTAG)
		.def("RemoveAll",			&CUIPropertiesBox::RemoveAll)
		.def("Show",				(void(CUIPropertiesBox::*)(int,int))&CUIPropertiesBox::Show)
		.def("Hide",				&CUIPropertiesBox::Hide)
//		.def("GetClickedIndex",		&CUIPropertiesBox::GetClickedIndex)
		.def("AutoUpdateSize",		&CUIPropertiesBox::AutoUpdateSize)
		.def("AddItem",				&CUIPropertiesBox::AddItem_script)
//		.def("",					&CUIPropertiesBox::)
	];
}
