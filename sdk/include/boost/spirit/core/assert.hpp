/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_ASSERT_HPP)
#define BOOST_SPIRIT_ASSERT_HPP

#include "boost/config.hpp"
#include "boost/throw_exception.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  BOOST_SPIRIT_ASSERT is used throughout the framework.  It can be
//  overridden by the user.  If BOOST_SPIRIT_ASSERT_EXCEPTION is defined,
//  then that will be thrown, otherwise, BOOST_SPIRIT_ASSERT simply turns
//  into a plain assert()
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_ASSERT)
#if defined(NDEBUG)
    #define BOOST_SPIRIT_ASSERT(x)
#elif defined (BOOST_SPIRIT_ASSERT_EXCEPTION)
    #define BOOST_SPIRIT_ASSERT_AUX(f, l, x) \
    do{ if (!(x)) boost::throw_exception(BOOST_SPIRIT_ASSERT_EXCEPTION(f "(" #l "): " #x)); } while(0)
    #define BOOST_SPIRIT_ASSERT(x) SPIRIT_ASSERT_AUX(__FILE__, __LINE__, x)
#else
    #include <cassert>
    #define BOOST_SPIRIT_ASSERT(x) assert(x)
#endif
#endif // !defined(BOOST_SPIRIT_ASSERT)

#endif // BOOST_SPIRIT_ASSERT_HPP
