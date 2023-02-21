////////////////////////////////////////////////////////////////////////////
//	Module 		: object_item_script.h
//	Created 	: 27.05.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object item script class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_factory_space.h"
#include "object_item_abstract.h"
#include "xrScriptEngine/Functor.hpp"

class CObjectItemScript : public CObjectItemAbstract
{
protected:
    typedef CObjectItemAbstract inherited;

protected:
    mutable luabind::functor<ObjectFactory::ClientObjectBaseClass*, luabind::policy::adopt<0>> m_client_creator;
    mutable luabind::functor<ObjectFactory::ServerObjectBaseClass*, luabind::policy::adopt<0>> m_server_creator;

public:
    CObjectItemScript(luabind::object client_creator,luabind::object server_creator,
        const CLASS_ID& clsid, LPCSTR script_clsid);

    CObjectItemScript(luabind::object creator, const CLASS_ID& clsid, LPCSTR script_clsid);
    virtual ObjectFactory::ClientObjectBaseClass* client_object() const;

    virtual ObjectFactory::ServerObjectBaseClass* server_object(LPCSTR section) const;
};
