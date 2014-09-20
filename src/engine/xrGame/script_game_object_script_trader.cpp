////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object_script3.cpp
//	Created 	: 25.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script game object script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"

using namespace luabind;

class_<CScriptGameObject> &script_register_game_object_trader(class_<CScriptGameObject> &instance)
{
	instance
		.def("set_trader_global_anim",	&CScriptGameObject::set_trader_global_anim)
		.def("set_trader_head_anim",	&CScriptGameObject::set_trader_head_anim)
		.def("set_trader_sound",		&CScriptGameObject::set_trader_sound)
		.def("external_sound_start",	&CScriptGameObject::external_sound_start)
		.def("external_sound_stop",		&CScriptGameObject::external_sound_stop)
	;return	(instance);
}
