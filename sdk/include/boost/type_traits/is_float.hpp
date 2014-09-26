
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TYPE_TRAITS_IS_FLOAT_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_IS_FLOAT_HPP_INCLUDED

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

namespace boost {

//* is a type T a floating-point type described in the standard (3.9.1p8)
BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_float,T,false)
BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(is_float,float,true)
BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(is_float,double,true)
BOOST_TT_AUX_BOOL_TRAIT_CV_SPEC1(is_float,long double,true)

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TYPE_TRAITS_IS_FLOAT_HPP_INCLUDED
