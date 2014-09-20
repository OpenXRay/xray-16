#include "pch_script.h"
#include "StalkerOutfit.h"
#include "ActorHelmet.h"

CStalkerOutfit::CStalkerOutfit()
{
}

CStalkerOutfit::~CStalkerOutfit() 
{
}

using namespace luabind;

#pragma optimize("s",on)
void CStalkerOutfit::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CStalkerOutfit,CGameObject>("CStalkerOutfit")
			.def(constructor<>()),

		class_<CHelmet,CGameObject>("CHelmet")
			.def(constructor<>())
	];
}
