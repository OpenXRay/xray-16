////////////////////////////////////////////////////////////////////////////
//	Module 		: object_item_script.cpp
//	Created 	: 27.05.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object item script class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "object_item_script.h"
#include "object_factory.h"
#include "xrScriptEngine/Functor.hpp"

#ifndef NO_XR_GAME
#	include "attachable_item.h"

ObjectFactory::ClientObjectBaseClass *CObjectItemScript::client_object	() const
{
	ObjectFactory::ClientObjectBaseClass *object = nullptr;
	try {
		object	= m_client_creator();
	}
	catch(...) {
		return	(0);
	}
	R_ASSERT	(object);
	return		(object->_construct());
}

#endif

ObjectFactory::ServerObjectBaseClass *CObjectItemScript::server_object	(LPCSTR section) const
{
    ObjectFactory::ServerObjectBaseClass *object = nullptr;

	try {
        object = m_server_creator(section);
	}
	catch(std::exception e) {
		Msg			("Exception [%s] raised while creating server object from section [%s]", e.what(),section);
		return		(0);
	}
	catch(...) {
		Msg			("Exception raised while creating server object from section [%s]",section);
		return		(0);
	}

	R_ASSERT			(object);
    object = object->init();
	R_ASSERT			(object);
	return				(object);
}

CObjectItemScript::CObjectItemScript	(
#ifndef NO_XR_GAME
	luabind::object		client_creator, 
#endif
	luabind::object		server_creator, 
	const CLASS_ID		&clsid, 
	LPCSTR				script_clsid
) : 
	inherited								(clsid,script_clsid)
{
#ifndef NO_XR_GAME
	m_client_creator						= client_creator;
#endif
	m_server_creator						= server_creator;
}

#ifndef NO_XR_GAME

CObjectItemScript::CObjectItemScript	(
	luabind::object		unknown_creator, 
	const CLASS_ID		&clsid, 
	LPCSTR				script_clsid
) : 
	inherited								(clsid,script_clsid)
{
	m_client_creator = m_server_creator		= unknown_creator;
}

#endif
