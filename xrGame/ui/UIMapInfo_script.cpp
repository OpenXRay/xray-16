#include "pch_script.h"
#include "UIMapInfo.h"

using namespace luabind;

#pragma optimize("s",on)
void CUIMapInfo::script_register(lua_State *L){
	module(L)
	[
		class_<CUIMapInfo, CUIWindow>("CUIMapInfo")
		.def(				constructor<>())
		.def("Init",		&CUIMapInfo::InitMapInfo)
		.def("InitMap",		&CUIMapInfo::InitMap)
	];

}