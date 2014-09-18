////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_brain_script.cpp
//	Created 	: 02.11.2005
//  Modified 	: 02.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster brain class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_monster_brain.h"
#include "alife_monster_movement_manager.h"

using namespace luabind;

CALifeMonsterMovementManager *get_movement	(const CALifeMonsterBrain *brain)
{
	return	(&brain->movement());
}

#pragma optimize("s",on)
void CALifeMonsterBrain::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CALifeMonsterBrain>("CALifeMonsterBrain")
			.def("movement",				&get_movement)
			.def("update",					&CALifeMonsterBrain::update)
			.def("can_choose_alife_tasks",	(void (CALifeMonsterBrain::*)(bool))&CALifeMonsterBrain::can_choose_alife_tasks)
	];
}
