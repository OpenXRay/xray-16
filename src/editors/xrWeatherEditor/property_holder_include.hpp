////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder_include.hpp
//	Created 	: 04.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property holder correct include
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_HOLDER_INCLUDE_HPP_INCLUDED
#define PROPERTY_HOLDER_INCLUDE_HPP_INCLUDED

#pragma unmanaged
#include "Common/Noncopyable.hpp"
#include "xrCore/fastdelegate.h"
#include <utility>
#include "Include/editor/property_holder_base.hpp"
#pragma managed

generic<typename type0, typename type1> private ref struct Pair
{
    type0 first;
    type1 second;
};

template <typename T>
class value_holder : private Noncopyable
{
public:
    inline value_holder(T& value) : m_value(value) {}
    inline T const& get() { return (m_value); }
    inline void set(T const& value) { m_value = value; }
private:
    T& m_value;
};

#endif // ifndef PROPERTY_HOLDER_INCLUDE_HPP_INCLUDED
