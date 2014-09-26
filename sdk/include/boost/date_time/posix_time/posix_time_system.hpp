

#ifndef POSIX_TIME_SYSTEM_HPP___
#define POSIX_TIME_SYSTEM_HPP___
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland
 */

#include "boost/date_time/posix_time/posix_time_config.hpp"
#include "boost/date_time/time_system_split.hpp"
#include "boost/date_time/time_system_counted.hpp"
#include "boost/date_time/compiler_config.hpp"


namespace boost {
namespace posix_time { 

#ifdef BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

#if (defined(BOOST_DATE_TIME_NO_MEMBER_INIT)) //help bad compilers 
  typedef date_time::split_timedate_system<posix_time_system_config, 1000000000> posix_time_system;
#else
  typedef date_time::split_timedate_system<posix_time_system_config> posix_time_system;
#endif

#else

  typedef date_time::counted_time_rep<millisec_posix_time_system_config> int64_time_rep;
  typedef date_time::counted_time_system<int64_time_rep> posix_time_system;

#endif

} }//namespace posix_time

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
 *
 */

#endif




























