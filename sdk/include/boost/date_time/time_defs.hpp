
#ifndef DATE_TIME_TIME_PRECISION_LIMITS_HPP
#define DATE_TIME_TIME_PRECISION_LIMITS_HPP

/* Copyright (c) 2000, 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */


/*! \file time_defs.hpp 
  This file contains nice definitions for handling the resoluion of various time
  reprsentations.
*/

namespace boost {
namespace date_time {

  //!Defines some nice types for handling time level resolutions
  enum time_resolutions {sec, tenth, hundreth, milli, ten_thousandth, micro, nano, NumResolutions };

  //! Flags for daylight savings or summer time
  enum dst_flags {not_dst, is_dst, calculate};


  //Todo move resolution strings
  //!A resolution adjustment table which corresponds to the name table
  //  static const unsigned long resolution_adjust[NumResolutions] = {1,10,100,1000,1000000,1000000000 };
  //!Some strings for the various resolutions
  //const char* const resolution_names[NumResolutions] = {"Second", "Tenth", "Hundreth", "Milli", "Micro", "Nano"};
 

} } //namespace date_time

/* Copyright (c) 2000,2002
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
