/*
 *
 * Copyright (c) 1998-2002
 * Dr John Maddock
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Dr John Maddock makes no representations
 * about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_fwd.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Forward declares boost::reg_expression<> and
  *                associated typedefs.
  */

#ifndef BOOST_REGEX_FWD_HPP_INCLUDED
#define BOOST_REGEX_FWD_HPP_INCLUDED

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/config.hpp>
#endif
#include <boost/detail/allocator.hpp>

//
// define BOOST_REGEX_NO_FWD if this
// header doesn't work!
//
#ifdef BOOST_REGEX_NO_FWD
#  ifndef BOOST_RE_REGEX_HPP
#     include <boost/regex.hpp>
#  endif
#else

//
// If there isn't good enough wide character support then there will
// be no wide character regular expressions:
//
#if (defined(BOOST_NO_CWCHAR) || defined(BOOST_NO_CWCTYPE) || defined(BOOST_NO_STD_WSTRING)) && !defined(BOOST_NO_WREGEX)
#  define BOOST_NO_WREGEX
#endif

namespace boost{

template <class charT>
class regex_traits;

template <class charT, class traits = regex_traits<charT>, class Allocator = BOOST_DEFAULT_ALLOCATOR(charT) >
class reg_expression;

typedef reg_expression<char, regex_traits<char>, BOOST_DEFAULT_ALLOCATOR(char) > regex;
#ifndef BOOST_NO_WREGEX
typedef reg_expression<wchar_t, regex_traits<wchar_t>, BOOST_DEFAULT_ALLOCATOR(wchar_t) > wregex;
#endif

} // namespace boost

#endif  // BOOST_REGEX_NO_FWD

#endif



