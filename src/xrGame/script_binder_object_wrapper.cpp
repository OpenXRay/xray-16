////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object_wrapper.cpp
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder wrapper
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_binder_object_wrapper.h"
#include "script_game_object.h"
#include "xrServer_Objects_ALife.h"

ScriptObjectBinderWrapper::ScriptObjectBinderWrapper(CScriptGameObject *object) :
    ScriptObjectBinder(object)
{
}

ScriptObjectBinderWrapper::~ScriptObjectBinderWrapper()
{
}

void ScriptObjectBinderWrapper::reinit					()
{
	luabind::call_member<void>		(this,"reinit");
}

void ScriptObjectBinderWrapper::reinit_static			(ScriptObjectBinder *script_binder_object)
{
	script_binder_object->ScriptObjectBinder::reinit	();
}

void ScriptObjectBinderWrapper::reload					(LPCSTR section)
{
	luabind::call_member<void>		(this,"reload",section);
}

void ScriptObjectBinderWrapper::reload_static			(ScriptObjectBinder *script_binder_object, LPCSTR section)
{
	script_binder_object->ScriptObjectBinder::reload	(section);
}

bool ScriptObjectBinderWrapper::net_Spawn				(SpawnType DC)
{
	return							(luabind::call_member<bool>(this,"net_spawn",DC));
}

bool ScriptObjectBinderWrapper::net_Spawn_static		(ScriptObjectBinder *script_binder_object, SpawnType DC)
{
	return							(script_binder_object->ScriptObjectBinder::net_Spawn(DC));
}

void ScriptObjectBinderWrapper::net_Destroy			()
{
	luabind::call_member<void>		(this,"net_destroy");
}

void ScriptObjectBinderWrapper::net_Destroy_static		(ScriptObjectBinder *script_binder_object)
{
	script_binder_object->ScriptObjectBinder::net_Destroy();
}

void ScriptObjectBinderWrapper::net_Import				(NET_Packet *net_packet)
{
	luabind::call_member<void>		(this,"net_import",net_packet);
}

void ScriptObjectBinderWrapper::net_Import_static		(ScriptObjectBinder *script_binder_object, NET_Packet *net_packet)
{
	script_binder_object->ScriptObjectBinder::net_Import	(net_packet);
}

void ScriptObjectBinderWrapper::net_Export				(NET_Packet *net_packet)
{
	luabind::call_member<void>		(this,"net_export",net_packet);
}

void ScriptObjectBinderWrapper::net_Export_static		(ScriptObjectBinder *script_binder_object, NET_Packet *net_packet)
{
	script_binder_object->ScriptObjectBinder::net_Export	(net_packet);
}

void ScriptObjectBinderWrapper::shedule_Update			(u32 time_delta)
{
	luabind::call_member<void>		(this,"update",time_delta);
}

void ScriptObjectBinderWrapper::shedule_Update_static	(ScriptObjectBinder *script_binder_object, u32 time_delta)
{
	script_binder_object->ScriptObjectBinder::shedule_Update	(time_delta);
}

void ScriptObjectBinderWrapper::save					(NET_Packet *output_packet)
{
	luabind::call_member<void>		(this,"save",output_packet);
}

void ScriptObjectBinderWrapper::save_static			(ScriptObjectBinder *script_binder_object, NET_Packet *output_packet)
{
	script_binder_object->ScriptObjectBinder::save		(output_packet);
}

void ScriptObjectBinderWrapper::load					(IReader *input_packet)
{
	luabind::call_member<void>		(this,"load",input_packet);
}

void ScriptObjectBinderWrapper::load_static			(ScriptObjectBinder *script_binder_object, IReader *input_packet)
{
	script_binder_object->ScriptObjectBinder::load		(input_packet);
}

bool ScriptObjectBinderWrapper::net_SaveRelevant		()
{
	return							(luabind::call_member<bool>(this,"net_save_relevant"));
}

bool ScriptObjectBinderWrapper::net_SaveRelevant_static(ScriptObjectBinder *script_binder_object)
{
	return							(script_binder_object->ScriptObjectBinder::net_SaveRelevant());
}

void ScriptObjectBinderWrapper::net_Relcase			(CScriptGameObject *object)
{
	luabind::call_member<void>		(this,"net_Relcase",object);
}

void ScriptObjectBinderWrapper::net_Relcase_static		(ScriptObjectBinder *script_binder_object, CScriptGameObject *object)
{
	script_binder_object->ScriptObjectBinder::net_Relcase	(object);
}