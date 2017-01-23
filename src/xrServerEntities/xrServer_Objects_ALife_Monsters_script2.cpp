////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Monsters_script2.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server monsters for ALife simulator, script export, the second part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CSE_ALifeCreatureCrow, (CSE_ALifeCreatureAbstract),
{
	module(luaState)
    [
		luabind_class_creature1(
			CSE_ALifeCreatureCrow,
			"cse_alife_creature_crow",
			CSE_ALifeCreatureAbstract
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeMonsterZombie, (CSE_ALifeMonsterAbstract),
{
	module(luaState)
    [
		luabind_class_monster1(
			CSE_ALifeMonsterZombie,
			"cse_alife_monster_zombie",
			CSE_ALifeMonsterAbstract
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeMonsterBase, (CSE_ALifeMonsterAbstract, CSE_PHSkeleton),
{
	module(luaState)
    [
		luabind_class_monster2(
			CSE_ALifeMonsterBase,
			"cse_alife_monster_base",
			CSE_ALifeMonsterAbstract,
			CSE_PHSkeleton
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeHumanStalker, (CSE_ALifeHumanAbstract, CSE_PHSkeleton),
{
	module(luaState)
    [
		luabind_class_monster2(
			CSE_ALifeHumanStalker,
			"cse_alife_human_stalker",
			CSE_ALifeHumanAbstract,
			CSE_PHSkeleton
		)
	];
});
