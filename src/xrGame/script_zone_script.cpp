////////////////////////////////////////////////////////////////////////////
//	Module 		: script_zone_script.cpp
//	Created 	: 10.10.2003
//  Modified 	: 11.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Script zone object script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_zone.h"
#include "smart_zone.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptZone, (IFactoryObject),
{
	module(luaState)
	[
		class_<CScriptZone,IFactoryObject>("ce_script_zone")
			.def(constructor<>())
	];
});

SCRIPT_EXPORT(CSmartZone, (IFactoryObject),
{
	module(luaState)
	[
		class_<CSmartZone,IFactoryObject>("ce_smart_zone")
			.def(constructor<>())
	];
});
