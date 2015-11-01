#include "pch_script.h"
#include "MosquitoBald.h"
#include "ZoneCampfire.h"
#include "TorridZone.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CMosquitoBald, (CGameObject),
{
	module(luaState)
	[	
		class_<CTorridZone,CGameObject>("CTorridZone")
			.def(constructor<>()),
		class_<CMosquitoBald,CGameObject>("CMosquitoBald")
			.def(constructor<>()),
		class_<CZoneCampfire,CGameObject>("CZoneCampfire")
			.def(constructor<>())
			.def("turn_on",				&CZoneCampfire::turn_on_script)
			.def("turn_off",			&CZoneCampfire::turn_off_script)
			.def("is_on",				&CZoneCampfire::is_on)
	];
});
