//-----------------------------------------------------------------------------
// boost mpl/aux_/config/preprocessor.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2000-02
// Aleksey Gurtovoy
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.

#ifndef BOOST_MPL_AUX_CONFIG_PREPROCESSOR_HPP_INCLUDED
#define BOOST_MPL_AUX_CONFIG_PREPROCESSOR_HPP_INCLUDED

#include "boost/config.hpp"

#if defined(__MWERKS__) && (__MWERKS__ <= 0x3003 || !defined(BOOST_STRICT_CONFIG)) \
 || defined(__BORLANDC__) && (__BORLANDC__ <= 0x561 || !defined(BOOST_STRICT_CONFIG)) \
 || defined(__IBMCPP__) && (__IBMCPP__ <= 502 || !defined(BOOST_STRICT_CONFIG))
#   define BOOST_MPL_BROKEN_PP_MACRO_EXPANSION
#endif

//#define BOOST_MPL_NO_OWN_PP_PRIMITIVES

#endif // BOOST_MPL_AUX_CONFIG_PREPROCESSOR_HPP_INCLUDED
