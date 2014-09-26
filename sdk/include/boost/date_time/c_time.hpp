
#ifndef DATE_TIME_C_TIME_HPP___
#define DATE_TIME_C_TIME_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

/*! @file c_time.hpp
  Provide workarounds related to the ctime header
*/

#include "boost/date_time/compiler_config.hpp"
#include <ctime>
//Work around libraries that don't put time_t and time in namespace std

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::time_t; using ::time; using ::localtime;
                using ::tm;  using ::gmtime; }
#endif

//The following is used to support high precision time clocks
#ifdef BOOST_HAS_GETTIMEOFDAY
#include <sys/time.h>
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
