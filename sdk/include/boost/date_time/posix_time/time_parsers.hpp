#ifndef POSIXTIME_PARSERS_HPP___
#define POSIXTIME_PARSERS_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/time_parsing.hpp"
#include "boost/date_time/posix_time/posix_time_types.hpp"

 
namespace boost {

namespace posix_time {

  inline time_duration duration_from_string(const std::string& s) {
    return date_time::parse_delimited_time_duration<time_duration>(s);
  }

  inline ptime time_from_string(const std::string& s) {
    return date_time::parse_delimited_time<ptime>(s, ' ');
  }

  inline ptime from_iso_string(const std::string& s) {
    return date_time::parse_iso_time<ptime>(s, 'T');
  }



} } //namespace posix_time


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

