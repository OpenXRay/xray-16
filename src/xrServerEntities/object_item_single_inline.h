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
ObjectFactory::CLIENT_BASE_CLASS *CSObjectItemSingle::client_object	() const
{
	FATAL				("Cannot instantiate client object, because client class is not declared!");
	return				(0);
}
#endif

TEMPLATE_SPECIALIZATION
ObjectFactory::SERVER_BASE_CLASS *CSObjectItemSingle::server_object	(LPCSTR section) const
{
	return				(xr_new<SERVER_TYPE>(section)->init());
}

#ifndef NO_XR_GAME
template <typename _unknown_type>
IC	CObjectItemSingle<_unknown_type,true>::CObjectItemSingle	(const CLASS_ID &clsid, LPCSTR script_clsid) :
	inherited			(clsid,script_clsid)
{
}

template <typename _unknown_type>
ObjectFactory::CLIENT_BASE_CLASS *CObjectItemSingle<_unknown_type,true>::client_object	() const
{
	return				(xr_new<CLIENT_TYPE>()->_construct());
}

template <typename _unknown_type>
ObjectFactory::SERVER_BASE_CLASS *CObjectItemSingle<_unknown_type,true>::server_object	(LPCSTR section) const
{
	FATAL				("Cannot instantiate server object, because server class is not declared!");
	return				(0);
}
#endif

#undef TEMPLATE_SPECIALIZATION
#undef CSObjectItemSingle
#endif