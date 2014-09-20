////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_script.cpp
//	Created 	: 27.05.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#ifndef DEDICATED_SERVER_ONLY

#include "object_factory.h"
#include "ai_space.h"
#include "script_engine.h"
#include "object_item_script.h"

void CObjectFactory::register_script_class	(LPCSTR client_class, LPCSTR server_class, LPCSTR clsid, LPCSTR script_clsid)
{
#ifndef NO_XR_GAME
	luabind::object				client;
	if (!ai().script_engine().function_object(client_class,client,LUA_TUSERDATA)) {
		ai().script_engine().script_log	(eLuaMessageTypeError,"Cannot register class %s",client_class);
		return;
	}
#endif
	luabind::object				server;
	if (!ai().script_engine().function_object(server_class,server,LUA_TUSERDATA)) {
		ai().script_engine().script_log	(eLuaMessageTypeError,"Cannot register class %s",server_class);
		return;
	}
	
	add							(
		xr_new<CObjectItemScript>(
#ifndef NO_XR_GAME
			client,
#endif
			server,
			TEXT2CLSID(clsid),
			script_clsid
		)
	);
}

void CObjectFactory::register_script_class			(LPCSTR unknown_class, LPCSTR clsid, LPCSTR script_clsid)
{
	luabind::object				creator;
	if (!ai().script_engine().function_object(unknown_class,creator,LUA_TUSERDATA)) {
		ai().script_engine().script_log	(eLuaMessageTypeError,"Cannot register class %s",unknown_class);
		return;
	}
	add							(
		xr_new<CObjectItemScript>(
#ifndef NO_XR_GAME
			creator,
#endif
			creator,
			TEXT2CLSID(clsid),
			script_clsid
		)
	);
}

#ifndef NO_XR_GAME
	ENGINE_API	bool g_dedicated_server;
#endif // NO_XR_GAME

void CObjectFactory::register_script_classes()
{
#ifndef NO_XR_GAME
	if (!g_dedicated_server)
#endif // NO_XR_GAME
		ai();
}

using namespace luabind;

struct CInternal{};

void CObjectFactory::register_script	() const
{
	actualize					();

	luabind::class_<CInternal>	instance("clsid");

	const_iterator				I = clsids().begin(), B = I;
	const_iterator				E = clsids().end();
	for ( ; I != E; ++I)
		instance.enum_			("_clsid")[luabind::value(*(*I)->script_clsid(),int(I - B))];

	luabind::module				(ai().script_engine().lua())[instance];
}

#pragma optimize("s",on)
void CObjectFactory::script_register(lua_State *L)
{
	module(L)
	[
		class_<CObjectFactory>("object_factory")
			.def("register",	(void (CObjectFactory::*)(LPCSTR,LPCSTR,LPCSTR,LPCSTR))(&CObjectFactory::register_script_class))
			.def("register",	(void (CObjectFactory::*)(LPCSTR,LPCSTR,LPCSTR))(&CObjectFactory::register_script_class))
	];
}

#endif // #ifndef DEDICATED_SERVER_ONLY