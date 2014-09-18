#include "pch_script.h"
#include "pseudo_gigant.h"

using namespace luabind;

#pragma optimize("s",on)
void CPseudoGigant::script_register(lua_State *L)
{
	module(L)
	[
		class_<CPseudoGigant,CGameObject>("CPseudoGigant")
			.def(constructor<>())
	];
}
