////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_script.cpp
//	Created 	: 27.05.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "object_factory.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "object_item_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

void CObjectFactory::register_script_class(LPCSTR client_class, LPCSTR server_class, LPCSTR clsid, LPCSTR script_clsid)
{
#ifdef CONFIG_OBJECT_FACTORY_LOG_REGISTER
    Msg("* CObjectFactory: registering script class '%s'", clsid);
#endif
    luabind::object client;
    if (!GEnv.ScriptEngine->function_object(client_class, client, LUA_TUSERDATA))
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Cannot register class %s", client_class);
        return;
    }

    luabind::object server;
    if (!GEnv.ScriptEngine->function_object(server_class, server, LUA_TUSERDATA))
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Cannot register class %s", server_class);
        return;
    }

    add(xr_new<CObjectItemScript>(client, server, TEXT2CLSID(clsid), script_clsid));
}

void CObjectFactory::register_script_class(LPCSTR unknown_class, LPCSTR clsid, LPCSTR script_clsid)
{
#ifdef CONFIG_OBJECT_FACTORY_LOG_REGISTER
    Msg("* CObjectFactory: registering script class '%s'", clsid);
#endif
    luabind::object creator;
    if (!GEnv.ScriptEngine->function_object(unknown_class, creator, LUA_TUSERDATA))
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Cannot register class %s", unknown_class);
        return;
    }
    add(xr_new<CObjectItemScript>(creator, creator, TEXT2CLSID(clsid), script_clsid));
}

void CObjectFactory::register_script_classes()
{
    if (!GEnv.isDedicatedServer)
        ai();
}

struct CInternal
{
};

void CObjectFactory::register_script() const
{
    actualize();

    luabind::class_<CInternal> instance("clsid");

    const_iterator I = clsids().begin(), B = I;
    const_iterator E = clsids().end();
    for (; I != E; ++I)
        instance.enum_("_clsid")[luabind::value(*(*I)->script_clsid(), int(I - B))];

    luabind::module(GEnv.ScriptEngine->lua())[instance];
}

SCRIPT_EXPORT(CObjectFactory, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CObjectFactory>("object_factory")
        .def("register", (void (CObjectFactory::*)(LPCSTR, LPCSTR, LPCSTR, LPCSTR))(
                             &CObjectFactory::register_script_class))
        .def("register", (void (CObjectFactory::*)(LPCSTR, LPCSTR, LPCSTR))(
                             &CObjectFactory::register_script_class))
    ];
});
