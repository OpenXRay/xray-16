////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_crow_script.cpp
//	Created 	: 24.12.2007
//  Modified 	: 24.12.2007
//	Author		: Alexander Dudin
//	Description : Crow script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai/crow/ai_crow.h"

using namespace luabind;

#pragma optimize("s",on)
void CAI_Crow::script_register(lua_State *L)
{
	module(L)
	[
		class_<CAI_Crow,CGameObject>("CAI_Crow")
			.def(constructor<>())
	];
}
