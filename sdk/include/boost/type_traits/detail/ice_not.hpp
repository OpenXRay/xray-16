// (C) Copyright John Maddock and Steve Cleary 2000.
//
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_DETAIL_ICE_NOT_HPP_INCLUDED
#define BOOST_TT_DETAIL_ICE_NOT_HPP_INCLUDED

#include "boost/config.hpp"

namespace boost {
namespace type_traits {

template <bool b>
struct ice_not
{
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct ice_not<true>
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

} // namespace type_traits
} // namespace boost

#endif // BOOST_TT_DETAIL_ICE_NOT_HPP_INCLUDED
