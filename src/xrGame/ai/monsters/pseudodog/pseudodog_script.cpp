#include "pch_script.h"
#include "pseudodog.h"
#include "psy_dog.h"

using namespace luabind;

#pragma optimize("s",on)
void CAI_PseudoDog::script_register(lua_State *L)
{
	module(L)
	[
		class_<CAI_PseudoDog,CGameObject>("CAI_PseudoDog")
			.def(constructor<>())
	];
}

void CPsyDog::script_register(lua_State *L)
{
	module(L)
		[
			class_<CPsyDog,CGameObject>("CPsyDog")
			.def(constructor<>())
		];
}

void CPsyDogPhantom::script_register(lua_State *L)
{
	module(L)
		[
			class_<CPsyDogPhantom,CGameObject>("CPsyDogPhantom")
			.def(constructor<>())
		];
}
