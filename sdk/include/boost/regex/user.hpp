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
  *   FILE         user.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: User settable options.
  */

// define if you want the regex library to use the C locale
// even on Win32:
// #define BOOST_REGEX_USE_C_LOCALE

// define this is you want the regex library to use the C++
// locale:
// #define BOOST_REGEX_USE_CPP_LOCALE

// define this if you want to statically link to regex,
// even when the runtime is a dll (Probably Win32 specific):
// #define BOOST_REGEX_STATIC_LINK

// define this if you don't want the lib to automatically
// select its link libraries:
// #define BOOST_REGEX_NO_LIB

// define this if templates with switch statements cause problems:
// #define BOOST_REGEX_NO_TEMPLATE_SWITCH_MERGE
 
// define this to disable Win32 support when available:
// #define BOOST_REGEX_NO_W32

// define this if bool is not a real type:
// #define BOOST_REGEX_NO_BOOL

// define this if no template instances are to be placed in
// the library rather than users object files:
// #define BOOST_REGEX_NO_EXTERNAL_TEMPLATES

// define this if the forward declarations in regex_fwd.hpp
// cause more problems than they are worth:
// #define BOOST_REGEX_NO_FWD



