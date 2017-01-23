////////////////////////////////////////////////////////////////////////////
//	Module 		: object_item_single_inline.h
//	Created 	: 27.05.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object item client or server class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef object_item_single_inlineH
#define object_item_single_inlineH

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename _unknown_type, bool _client_object>
#define CSObjectItemSingle CObjectItemSingle<_unknown_type,_client_object>

TEMPLATE_SPECIALIZATION
IC	CSObjectItemSingle::CObjectItemSingle	(const CLASS_ID &clsid, LPCSTR script_clsid) :
	inherited			(clsid,script_clsid)
{
}

#ifndef NO_XR_GAME
TEMPLATE_SPECIALIZATION
ObjectFactory::ClientObjectBaseClass *CSObjectItemSingle::client_object	() const
{
	FATAL				("Cannot instantiate client object, because client class is not declared!");
	return				(0);
}
#endif

TEMPLATE_SPECIALIZATION
ObjectFactory::ServerObjectBaseClass *CSObjectItemSingle::server_object	(LPCSTR section) const
{
	return				(new SERVER_TYPE(section))->init();
}

#ifndef NO_XR_GAME
template <typename _unknown_type>
IC	CObjectItemSingle<_unknown_type,true>::CObjectItemSingle	(const CLASS_ID &clsid, LPCSTR script_clsid) :
	inherited			(clsid,script_clsid)
{
}

template <typename _unknown_type>
ObjectFactory::ClientObjectBaseClass *CObjectItemSingle<_unknown_type,true>::client_object	() const
{
	return				(new CLIENT_TYPE())->_construct();
}

template <typename _unknown_type>
ObjectFactory::ServerObjectBaseClass *CObjectItemSingle<_unknown_type,true>::server_object	(LPCSTR section) const
{
	FATAL				("Cannot instantiate server object, because server class is not declared!");
	return				(0);
}
#endif

#undef TEMPLATE_SPECIALIZATION
#undef CSObjectItemSingle
#endif