#include "pch_script.h"
#include "dog.h"

using namespace luabind;

#pragma optimize("s",on)
void CAI_Dog::script_register(lua_State *L)
{
	module(L)
	[
		class_<CAI_Dog,CGameObject>("CAI_Dog")
			.def(constructor<>())
	];
}
