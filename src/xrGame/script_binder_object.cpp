////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object.cpp
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_binder_object.h"
#include "script_game_object.h"

ScriptObjectBinder::ScriptObjectBinder(CScriptGameObject *object)
{
	m_object		= object;
}

ScriptObjectBinder::~ScriptObjectBinder()
{
#ifdef DEBUG
	if (m_object)
		Msg			("Destroying binded object %s",m_object->Name());
#endif
}

void ScriptObjectBinder::reinit			()
{
}

void ScriptObjectBinder::reload			(LPCSTR section)
{
}

bool ScriptObjectBinder::net_Spawn			(SpawnType DC)
{
	return			(true);
}

void ScriptObjectBinder::net_Destroy		()
{
}

void ScriptObjectBinder::net_Import		(NET_Packet *net_packet)
{
}

void ScriptObjectBinder::net_Export		(NET_Packet *net_packet)
{
}

void ScriptObjectBinder::shedule_Update	(u32 time_delta)
{
}

void ScriptObjectBinder::save				(NET_Packet *output_packet)
{
}

void ScriptObjectBinder::load				(IReader	*input_packet)
{
}

bool ScriptObjectBinder::net_SaveRelevant	()
{
	return		(false);
}

void ScriptObjectBinder::net_Relcase		(CScriptGameObject *object)
{
}
