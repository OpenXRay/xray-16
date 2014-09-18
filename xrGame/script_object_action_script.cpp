////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object_action_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object action class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_object_action.h"
#include "script_game_object.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptObjectAction::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptObjectAction>("object")
			.enum_("state")
			[
				value("idle",					int(MonsterSpace::eObjectActionIdle)),
				value("show",					int(MonsterSpace::eObjectActionShow)),		
				value("hide",					int(MonsterSpace::eObjectActionHide)),		
				value("take",					int(MonsterSpace::eObjectActionTake)),		
				value("drop",					int(MonsterSpace::eObjectActionDrop)),		
				value("strap",					int(MonsterSpace::eObjectActionStrapped)),		
				value("aim1",					int(MonsterSpace::eObjectActionAim1)),		
				value("aim2",					int(MonsterSpace::eObjectActionAim2)),		
				value("reload",					int(MonsterSpace::eObjectActionReload1)),	
				value("reload1",				int(MonsterSpace::eObjectActionReload1)),	
				value("reload2",				int(MonsterSpace::eObjectActionReload2)),	
				value("fire1",					int(MonsterSpace::eObjectActionFire1)),		
				value("fire2",					int(MonsterSpace::eObjectActionFire2)),		
				value("switch1",				int(MonsterSpace::eObjectActionSwitch1)),	
				value("switch2",				int(MonsterSpace::eObjectActionSwitch2)),	
				value("activate",				int(MonsterSpace::eObjectActionActivate)),
				value("deactivate",				int(MonsterSpace::eObjectActionDeactivate)),
				value("use",					int(MonsterSpace::eObjectActionUse)),
				value("turn_on",				int(MonsterSpace::eObjectActionTurnOn)),
				value("turn_off",				int(MonsterSpace::eObjectActionTurnOff)),
				value("dummy",					int(MonsterSpace::eObjectActionDummy))
			]
			.def(								constructor<>())
			.def(								constructor<CScriptGameObject*,MonsterSpace::EObjectAction>())
			.def(								constructor<CScriptGameObject*,MonsterSpace::EObjectAction,u32>())
			.def(								constructor<MonsterSpace::EObjectAction>())
			.def(								constructor<LPCSTR,MonsterSpace::EObjectAction>())
			.def("action",						&CScriptObjectAction::SetObjectAction)
			.def("object",						(void (CScriptObjectAction::*)(LPCSTR))(&CScriptObjectAction::SetObject))
			.def("object",						(void (CScriptObjectAction::*)(CScriptGameObject*))(&CScriptObjectAction::SetObject))
			.def("completed",					(bool (CScriptObjectAction::*)())(&CScriptObjectAction::completed))
	];
}
