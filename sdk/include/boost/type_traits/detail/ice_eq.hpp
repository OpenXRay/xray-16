// (C) Copyright John Maddock and Steve Cleary 2000.
//
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_DETAIL_ICE_EQ_HPP_INCLUDED
#define BOOST_TT_DETAIL_ICE_EQ_HPP_INCLUDED

#include "boost/config.hpp"

namespace boost {
namespace type_traits {

template <int b1, int b2>
struct ice_eq
{
    BOOST_STATIC_CONSTANT(bool, value = (b1 == b2));
};

template <int b1, int b2>
struct ice_ne
{
    BOOST_STATIC_CONSTANT(bool, value = (b1 != b2));
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
template <int b1, int b2> bool const ice_eq<b1,b2>::value;
template <int b1, int b2> bool const ice_ne<b1,b2>::value;
#endif

} // namespace type_traits
} // namespace boost

#endif // BOOST_TT_DETAIL_ICE_EQ_HPP_INCLUDED
