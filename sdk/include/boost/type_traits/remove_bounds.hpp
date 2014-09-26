
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_BOUNDS_HPP_INCLUDED
#define BOOST_TT_REMOVE_BOUNDS_HPP_INCLUDED

#include "boost/config.hpp"
#include <cstddef>

// should be the last #include
#include "boost/type_traits/detail/type_trait_def.hpp"

namespace boost {

BOOST_TT_AUX_TYPE_TRAIT_DEF1(remove_bounds,T,T)

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_bounds,T[N],T type)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_bounds,T const[N],T const type)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_bounds,T volatile[N],T volatile type)
BOOST_TT_AUX_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T,std::size_t N,remove_bounds,T const volatile[N],T const volatile type)
#endif

} // namespace boost

#include "boost/type_traits/detail/type_trait_undef.hpp"

#endif // BOOST_TT_REMOVE_BOUNDS_HPP_INCLUDED
