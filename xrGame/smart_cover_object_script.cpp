////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_script.cpp
//	Created 	: 21.12.2007
//  Modified 	: 21.12.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover.h"

using namespace luabind;

#pragma optimize("s",on)
void smart_cover::object::script_register	(lua_State *L)
{
	module(L) [
		class_<smart_cover::object,CGameObject>("smart_cover_object")
			.def(constructor<>())
	];
}
