#ifndef DATE_TIME_LOCALE_CONFIG_HPP___
#define DATE_TIME_LOCALE_CONFIG_HPP____
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

// This file configures whether the library will support locales and hence
// iostream based i/o.  Even if a compiler has some support for locales,
// any failure to be compatible gets the compiler on the exclusion list.
//
// At the moment this is defined for MSVC 6 and any compiler that 
// defines BOOST_NO_STD_LOCALE (gcc 2.95.x)

#include "boost/config.hpp" //sets BOOST_NO_STD_LOCALE

//This file basically becomes a noop if locales are not properly supported
#if (defined(BOOST_NO_STD_LOCALE) || (defined(BOOST_MSVC) && (_MSC_VER <= 1200)) || (defined(__BORLANDC__)))
#define BOOST_DATE_TIME_NO_LOCALE
#endif



/* Copyright (c) 2002
 * CrystalClear Software, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  CrystalClear Software makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */
#endif
