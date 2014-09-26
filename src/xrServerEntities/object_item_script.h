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

class CObjectItemScript : public CObjectItemAbstract {
protected:
	typedef CObjectItemAbstract inherited;

protected:
	luabind::object								m_client_creator;
	luabind::object								m_server_creator;

public:
												CObjectItemScript	(
#ifndef NO_XR_GAME
													luabind::object client_creator, 
#endif
													luabind::object server_creator, 
													const CLASS_ID &clsid, 
													LPCSTR script_clsid
												);
#ifndef NO_XR_GAME
												CObjectItemScript	(
													luabind::object creator, 
													const CLASS_ID &clsid, 
													LPCSTR script_clsid
												);
	virtual ObjectFactory::CLIENT_BASE_CLASS	*client_object		() const;
#endif
	virtual ObjectFactory::SERVER_BASE_CLASS	*server_object		(LPCSTR section) const;
};
