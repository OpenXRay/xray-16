
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_IS_FUNDAMENTAL_HPP_INCLUDED
#define BOOST_TT_IS_FUNDAMENTAL_HPP_INCLUDED

#include "boost/type_traits/is_arithmetic.hpp"
#include "boost/type_traits/is_void.hpp"
#include "boost/type_traits/detail/ice_or.hpp"

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

namespace boost {

namespace detail {

template <typename T> 
struct is_fundamental_impl
    : ::boost::type_traits::ice_or< 
          ::boost::is_arithmetic<T>::value
        , ::boost::is_void<T>::value
        >
{ 
};

} // namespace detail

//* is a type T a fundamental type described in the standard (3.9.1)
BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_fundamental,T,::boost::detail::is_fundamental_impl<T>::value)

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TT_IS_FUNDAMENTAL_HPP_INCLUDED
