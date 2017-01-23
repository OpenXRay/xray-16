////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_container_inline.h
//	Created 	: 01.07.2004
//  Modified 	: 01.07.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry container class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>
IC	T	&CALifeRegistryContainer::operator()		(const T*)
{
	const int		value = Loki::TL::IndexOf<TYPE_LIST,T>::value;
	STATIC_CHECK	(value != -1,There_is_no_specified_registry_in_the_registry_container);
	return			(*static_cast<T*>(this));
}

template <typename T>
IC	const T &CALifeRegistryContainer::operator()	(const T*) const
{
	const int		value = Loki::TL::IndexOf<TYPE_LIST,T>::value;
	STATIC_CHECK	(value != -1,There_is_no_specified_registry_in_the_registry_container);
	return			(*static_cast<T*>(this));
}
