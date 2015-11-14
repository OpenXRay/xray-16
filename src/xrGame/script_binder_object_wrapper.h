////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object_wrapper.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder wrapper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_binder_object.h"
#include "script_game_object.h"

class CScriptGameObject;

class ScriptObjectBinderWrapper : public ScriptObjectBinder, public luabind::wrap_base {
public:
    ScriptObjectBinderWrapper(CScriptGameObject *object);
	virtual				~ScriptObjectBinderWrapper();
	virtual void		reinit						();
	static  void		reinit_static				(ScriptObjectBinder *script_binder_object);
	virtual void		reload						(LPCSTR section);
	static  void		reload_static				(ScriptObjectBinder *script_binder_object, LPCSTR section);
	virtual bool		net_Spawn					(SpawnType DC);
	static  bool		net_Spawn_static			(ScriptObjectBinder *script_binder_object, SpawnType DC);
	virtual void		net_Destroy					();
	static  void		net_Destroy_static			(ScriptObjectBinder *script_binder_object);
	virtual void		net_Import					(NET_Packet *net_packet);
	static  void		net_Import_static			(ScriptObjectBinder *script_binder_object, NET_Packet *net_packet);
	virtual void		net_Export					(NET_Packet *net_packet);
	static  void		net_Export_static			(ScriptObjectBinder *script_binder_object, NET_Packet *net_packet);
	virtual void		shedule_Update				(u32 time_delta);
	static  void		shedule_Update_static		(ScriptObjectBinder *script_binder_object, u32 time_delta);
	virtual void		save						(NET_Packet *output_packet);
	static	void		save_static					(ScriptObjectBinder *script_binder_object, NET_Packet *output_packet);
	virtual void		load						(IReader *input_packet);
	static	void		load_static					(ScriptObjectBinder *script_binder_object, IReader *input_packet);
	virtual bool		net_SaveRelevant			();
	static  bool		net_SaveRelevant_static		(ScriptObjectBinder *script_binder_object);
	virtual void		net_Relcase					(CScriptGameObject *object);
	static	void		net_Relcase_static			(ScriptObjectBinder *script_binder_object, CScriptGameObject *object);
};
