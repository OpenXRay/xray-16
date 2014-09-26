//-----------------------------------------------------------------------------
// boost mpl/aux_/deref_wknd.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2001-02
// Aleksey Gurtovoy
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.

#ifndef BOOST_MPL_AUX_DEREF_WNKD_HPP_INCLUDED
#define BOOST_MPL_AUX_DEREF_WNKD_HPP_INCLUDED

#include "boost/mpl/aux_/config/eti.hpp"

#if defined(BOOST_MPL_MSVC_ETI_BUG)

namespace boost { namespace mpl { namespace aux {

template< typename Iterator >
struct deref_wknd
{
    typedef typename Iterator::type type;
};

template<> struct deref_wknd<int>
{
    typedef int type;
};

}}} // namespace boost::mpl::aux

#   define BOOST_MPL_AUX_DEREF_WNKD(iter) aux::deref_wknd<iter>::type

#else

#   define BOOST_MPL_AUX_DEREF_WNKD(iter) iter::type

#endif

#endif // BOOST_MPL_AUX_DEREF_WNKD_HPP_INCLUDED
