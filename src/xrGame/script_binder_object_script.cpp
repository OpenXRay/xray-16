////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object_script.cpp
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_binder_object.h"
#include "script_binder_object_wrapper.h"
#include "xrServer_Objects_ALife.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(ScriptObjectBinder, (),
{
	module(luaState)
	[
		class_<ScriptObjectBinder,ScriptObjectBinderWrapper>("object_binder")
			.def_readonly("object",				&ScriptObjectBinder::m_object)
			.def(								constructor<CScriptGameObject*>())
			.def("reinit",						&ScriptObjectBinder::reinit,			&ScriptObjectBinderWrapper::reinit_static)
			.def("reload",						&ScriptObjectBinder::reload,			&ScriptObjectBinderWrapper::reload_static)
			.def("net_spawn",					&ScriptObjectBinder::net_Spawn,		&ScriptObjectBinderWrapper::net_Spawn_static)
			.def("net_destroy",					&ScriptObjectBinder::net_Destroy,		&ScriptObjectBinderWrapper::net_Destroy_static)
			.def("net_import",					&ScriptObjectBinder::net_Import,		&ScriptObjectBinderWrapper::net_Import_static)
			.def("net_export",					&ScriptObjectBinder::net_Export,		&ScriptObjectBinderWrapper::net_Export_static)
			.def("update",						&ScriptObjectBinder::shedule_Update,	&ScriptObjectBinderWrapper::shedule_Update_static)
			.def("save",						&ScriptObjectBinder::save,				&ScriptObjectBinderWrapper::save_static)
			.def("load",						&ScriptObjectBinder::load,				&ScriptObjectBinderWrapper::load_static)
			.def("net_save_relevant",			&ScriptObjectBinder::net_SaveRelevant,	&ScriptObjectBinderWrapper::net_SaveRelevant_static)
			.def("net_Relcase",					&ScriptObjectBinder::net_Relcase,		&ScriptObjectBinderWrapper::net_Relcase_static)
	];
});
