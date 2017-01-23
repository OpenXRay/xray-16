#include "pch_script.h"
#include "StalkerOutfit.h"
#include "ActorHelmet.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CStalkerOutfit::CStalkerOutfit()
{
}

CStalkerOutfit::~CStalkerOutfit() 
{
}

using namespace luabind;

SCRIPT_EXPORT(CStalkerOutfit, (CGameObject),
{
	module(luaState)
	[
		class_<CStalkerOutfit,CGameObject>("CStalkerOutfit")
			.def(constructor<>()),

		class_<CHelmet,CGameObject>("CHelmet")
			.def(constructor<>())
	];
});
