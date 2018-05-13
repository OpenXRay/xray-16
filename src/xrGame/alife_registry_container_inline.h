////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_container_inline.h
//	Created 	: 01.07.2004
//  Modified 	: 01.07.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry container class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>
IC T& CALifeRegistryContainer::operator()(const T*)
{
    constexpr int value = Loki::TL::IndexOf<TYPE_LIST, T>::value;
    static_assert(value != -1, "There is no specified registry in the registry container");
    return (*static_cast<T*>(this));
}

template <typename T>
IC const T& CALifeRegistryContainer::operator()(const T*) const
{
    constexpr int value = Loki::TL::IndexOf<TYPE_LIST, T>::value;
    static_assert(value != -1, "There is no specified registry in the registry container");
    return (*static_cast<T*>(this));
}
