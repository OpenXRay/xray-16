
#ifndef BOOST_MPL_GREATER_HPP_INCLUDED
#define BOOST_MPL_GREATER_HPP_INCLUDED

// + file: boost/mpl/greater.hpp
// + last modified: 25/feb/03

// Copyright (c) 2000-03
// Aleksey Gurtovoy
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.
//
// See http://www.boost.org/libs/mpl for documentation.

#include "boost/mpl/bool.hpp"
#include "boost/mpl/integral_c.hpp"
#include "boost/mpl/aux_/value_wknd.hpp"
#include "boost/mpl/aux_/void_spec.hpp"
#include "boost/mpl/aux_/lambda_support.hpp"
#include "boost/config.hpp"

namespace boost {
namespace mpl {

template<
      typename BOOST_MPL_AUX_VOID_SPEC_PARAM(T1)
    , typename BOOST_MPL_AUX_VOID_SPEC_PARAM(T2)
    >
struct greater
{
    BOOST_STATIC_CONSTANT(bool, value = (
          (BOOST_MPL_AUX_VALUE_WKND(T1)::value)
            > (BOOST_MPL_AUX_VALUE_WKND(T2)::value)
        ));

#if !defined(__BORLANDC__)
    typedef bool_<value> type;
#else
    typedef bool_<(
          (BOOST_MPL_AUX_VALUE_WKND(T1)::value)
            > (BOOST_MPL_AUX_VALUE_WKND(T2)::value)
        )> type;
#endif

    BOOST_MPL_AUX_LAMBDA_SUPPORT(2,greater,(T1,T2))
};

BOOST_MPL_AUX_VOID_SPEC(2, greater)

template< long N >
struct gt
{
    template< typename T > struct apply
#if !defined(__BORLANDC__)
        : greater< T,integral_c<long,N> >
    {
#else
    {
        typedef typename greater< T,integral_c<long,N> >::type type;
#endif
    };
};

} // namespace mpl
} // namespace boost

#endif // BOOST_MPL_GREATER_HPP_INCLUDED
