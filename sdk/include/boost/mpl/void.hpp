//-----------------------------------------------------------------------------
// boost mpl/void.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2001-02
// Peter Dimov, Aleksey Gurtovoy
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appears in all copies and
// that both the copyright notice and this permission notice appear in
// supporting documentation. No representations are made about the
// suitability of this software for any purpose. It is provided "as is"
// without express or implied warranty.

#ifndef BOOST_MPL_VOID_HPP_INCLUDED
#define BOOST_MPL_VOID_HPP_INCLUDED

#include "boost/mpl/bool.hpp"
#include "boost/config.hpp"

namespace boost {
namespace mpl {

//  [JDG Feb-4-2003] made void_ a complete type to allow it to be
//  instantiated so that it can be passed in as an object that can be
//  used to select an overloaded function. Possible use includes signaling
//  a zero arity functor evaluation call.
struct void_ {};

template< typename T >
struct is_void_
    : false_
{
#if defined(BOOST_MSVC) && BOOST_MSVC < 1300
    using false_::value;
#endif
};

template<>
struct is_void_<void_>
    : true_
{
#if defined(BOOST_MSVC) && BOOST_MSVC < 1300
    using true_::value;
#endif
};

} // namespace mpl
} // namespace boost

#endif // BOOST_MPL_VOID_HPP_INCLUDED
