
// (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
// Hinnant & John Maddock 2000.  Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_IS_UNION_HPP_INCLUDED
#define BOOST_TT_IS_UNION_HPP_INCLUDED

#include "boost/type_traits/remove_cv.hpp"
#include "boost/type_traits/config.hpp"
#include "boost/type_traits/intrinsics.hpp"

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

namespace boost {

namespace detail {
template <typename T> struct is_union_impl
{
   typedef typename remove_cv<T>::type cvt;
   BOOST_STATIC_CONSTANT(bool, value = BOOST_IS_UNION(cvt));
};
} // namespace detail

BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_union,T,::boost::detail::is_union_impl<T>::value)

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TT_IS_UNION_HPP_INCLUDED
