////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Smartcovers_script.cpp
//	Created 	: 18.12.2008
//  Modified 	: 18.12.2008
//	Author		: Plichko Alexander
//	Description : Server smartcovers for ALife simulator, script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Smartcovers.h"
#include "xrServer_script_macroses.h"

using namespace luabind;
#pragma optimize("s",on)

void CSE_SmartCover::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife1(
			CSE_SmartCover,
			"cse_smart_cover",
			CSE_ALifeDynamicObject
			)
			.def	("description",	&CSE_SmartCover::description)
#ifndef AI_COMPILER
			.def	("set_available_loopholes", &CSE_SmartCover::set_available_loopholes )
#endif // #ifndef AI_COMPILER

#ifdef XRSE_FACTORY_EXPORTS
			.def	("set_loopholes_table_checker", &CSE_SmartCover::set_loopholes_table_checker )
#endif // #ifdef XRSE_FACTORY_EXPORTS
	];
}