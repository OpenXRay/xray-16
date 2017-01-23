////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_impl.h
//	Created 	: 27.05.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory implementation
////////////////////////////////////////////////////////////////////////////

#ifndef object_factory_implH
#define object_factory_implH

#pragma once

#include "object_factory.h"
#include "object_item_single.h"

#ifndef NO_XR_GAME
#	include "Common/object_type_traits.h"
#	include "object_item_client_server.h"
#endif // NO_XR_GAME

#ifndef NO_XR_GAME

template <typename _client_type, typename _server_type>
IC	void CObjectFactory::add	(const CLASS_ID &clsid, LPCSTR script_clsid)
{
	{
		typedef object_type_traits::is_base_and_derived<ClientObjectBaseClass,_client_type> a;
        static_assert(a::value, "Client object must derive ClientObjectBaseClass");
	}
	{
		typedef object_type_traits::is_base_and_derived<ServerObjectBaseClass,_server_type> a;
        static_assert(a::value, "Server object must derive ServerObjectBaseClass");
	}
	add					(new CObjectItemClientServer<_client_type,_server_type>(clsid,script_clsid));
}

template <typename _unknown_type>
IC	void CObjectFactory::add	(const CLASS_ID &clsid, LPCSTR script_clsid)
{
	{
		typedef object_type_traits::is_base_and_derived<ClientObjectBaseClass,_unknown_type> a;
		typedef object_type_traits::is_base_and_derived<ServerObjectBaseClass,_unknown_type> b;
        static_assert(a::value || b::value, "Class must derive ClientObjectBaseClass or ServerObjectBaseClass");
	}
	add					(
		new CObjectItemSingle<
				_unknown_type,
				object_type_traits::is_base_and_derived<ClientObjectBaseClass,_unknown_type>::value
			>
		(clsid,script_clsid)
	);
}

#else // NO_XR_GAME

template <typename _unknown_type>
IC	void CObjectFactory::add	(const CLASS_ID &clsid, LPCSTR script_clsid)
{
	add					(new CObjectItemSingle<_unknown_type,false>(clsid,script_clsid));
}

#endif // NO_XR_GAME

#endif // object_factory_implH