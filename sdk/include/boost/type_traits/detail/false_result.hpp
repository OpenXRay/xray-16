// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

#ifndef BOOST_TT_DETAIL_FALSE_RESULT_HPP_INCLUDED
#define BOOST_TT_DETAIL_FALSE_RESULT_HPP_INCLUDED

#include "boost/config.hpp"

namespace boost {
namespace type_traits {

// Utility class which always "returns" false
struct false_result
{
    template <typename T> struct result_
    {
        BOOST_STATIC_CONSTANT(bool, value = false);
    };
};

}} // namespace boost::type_traits

#endif // BOOST_TT_DETAIL_FALSE_RESULT_HPP_INCLUDED
