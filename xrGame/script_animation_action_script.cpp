////////////////////////////////////////////////////////////////////////////
//	Module 		: script_animation_action_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script animation action class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_animation_action.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptAnimationAction::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptAnimationAction>("anim")
			.enum_("type")
			[
				value("free",					int(MonsterSpace::eMentalStateFree)),
				value("danger",					int(MonsterSpace::eMentalStateDanger)),
				value("panic",					int(MonsterSpace::eMentalStatePanic))
			]
			.enum_("monster")
			[
				value("stand_idle",				int(MonsterSpace::eAA_StandIdle)),
				value("capture_prepare",		int(MonsterSpace::eAA_CapturePrepare)),
				value("sit_idle",				int(MonsterSpace::eAA_SitIdle)),
				value("lie_idle",				int(MonsterSpace::eAA_LieIdle)),
				value("eat",					int(MonsterSpace::eAA_Eat)),
				value("sleep",					int(MonsterSpace::eAA_Sleep)),
				value("rest",					int(MonsterSpace::eAA_Rest)),
				value("attack",					int(MonsterSpace::eAA_Attack)),
				value("look_around",			int(MonsterSpace::eAA_LookAround)),
				value("turn",					int(MonsterSpace::eAA_Turn))
			]

			.def(								constructor<>())
			.def(								constructor<LPCSTR>())
			.def(								constructor<LPCSTR,bool>())
			.def(								constructor<MonsterSpace::EMentalState>())
			
			// Monster specific
			.def(								constructor<MonsterSpace::EScriptMonsterAnimAction, int>())
			
			.def("anim",						&CScriptAnimationAction::SetAnimation)
			.def("type",						&CScriptAnimationAction::SetMentalState)
			.def("completed",					(bool (CScriptAnimationAction::*)())(&CScriptAnimationAction::completed))
	];
}