
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.  Permission to copy, use, modify,
//  sell and distribute this software is granted provided this
//  copyright notice appears in all copies. This software is provided
//  "as is" without express or implied warranty, and with no claim as
//  to its suitability for any purpose.
//
//  See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_ADD_CONST_HPP_INCLUDED
#define BOOST_TT_ADD_CONST_HPP_INCLUDED

#include "boost/config.hpp"

// should be the last #include
#include "boost/type_traits/detail/type_trait_def.hpp"

namespace boost {

// * convert a type T to const type - add_const<T>
// this is not required since the result is always
// the same as "T const", but it does suppress warnings
// from some compilers:

#if defined(BOOST_MSVC)
// This bogus warning will appear when add_const is applied to a
// const volatile reference because we can't detect const volatile
// references with MSVC6.
#   pragma warning(push)
#   pragma warning(disable:4181) // warning C4181: qualifier applied to reference type ignored
#endif 

BOOST_TT_AUX_TYPE_TRAIT_DEF1(add_const,T,T const)

#if defined(BOOST_MSVC)
#   pragma warning(pop)
#endif 

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T,add_const,T&,T&)
#endif

} // namespace boost

#include "boost/type_traits/detail/type_trait_undef.hpp"

#endif // BOOST_TT_ADD_CONST_HPP_INCLUDED
