//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.  Permission to copy, use, modify,
//  sell and distribute this software is granted provided this
//  copyright notice appears in all copies. This software is provided
//  "as is" without express or implied warranty, and with no claim as
//  to its suitability for any purpose.
//
//  See http://www.boost.org for most recent version including documentation.
//
//  defines traits classes for composite types:
//  is_array, is_pointer, is_reference, is_member_pointer, is_enum, is_union.
//
//    Fixed is_pointer, is_reference, is_const, is_volatile, is_same, 
//    is_member_pointer based on the Simulated Partial Specialization work 
//    of Mat Marcus and Jesse Jones. See  http://opensource.adobe.com or 
//    http://groups.yahoo.com/group/boost/message/5441 
//    Some workarounds in here use ideas suggested from "Generic<Programming>: 
//    Mappings between Types and Values" 
//    by Andrei Alexandrescu (see http://www.cuj.com/experts/1810/alexandr.html).
//    Fixes for is_array are based on a newgroup posting by Jonathan Lundquist.

#ifndef BOOST_TT_COMPOSITE_TRAITS_HPP_INCLUDED
#define BOOST_TT_COMPOSITE_TRAITS_HPP_INCLUDED

#include "boost/type_traits/is_array.hpp"
#include "boost/type_traits/is_enum.hpp"
#include "boost/type_traits/is_member_pointer.hpp"
#include "boost/type_traits/is_member_function_pointer.hpp"
#include "boost/type_traits/is_pointer.hpp"
#include "boost/type_traits/is_reference.hpp"
#include "boost/type_traits/is_union.hpp"

#endif // BOOST_TT_COMPOSITE_TRAITS_HPP_INCLUDED





