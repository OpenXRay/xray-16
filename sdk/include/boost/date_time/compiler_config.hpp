#ifndef DATE_TIME_COMPILER_CONFIG_HPP___
#define DATE_TIME_COMPILER_CONFIG_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 * $Date: 2003/02/19 12:25:03 $
 */

// This file performs some local compiler configurations

#include "boost/date_time/locale_config.hpp" //set up locale configurations

//Set up a configuration parameter for platforms that have 
//GetTimeOfDay
#ifdef BOOST_HAS_GETTIMEOFDAY
#define BOOST_DATE_TIME_HAS_GETTIMEOFDAY_HIGH_PRECISION_CLOCK
#endif


#if (defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION) || (defined(__BORLANDC__)))
#define BOOST_DATE_TIME_NO_MEMBER_INIT
#endif

//Define INT64_C for some Metrowerks compilers
#if (defined(__MWERKS__) && (!defined(INT64_C)))
#define INT64_C(value)  long long(value)
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
