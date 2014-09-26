#ifndef GREGORIAN_PARSERS_HPP___
#define GREGORIAN_PARSERS_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/gregorian_types.hpp"
#include "boost/date_time/date_parsing.hpp"


namespace boost {
namespace gregorian {

  //! From delimited date string where with order year-month-day eg: 2002-1-25
  inline date from_string(std::string s) {
    return date_time::parse_date<date>(s);
  }

  //! From iso type date string where with order year-month-day eg: 20020125
  inline date from_undelimited_string(std::string s) {
    return date_time::parse_undelimited_date<date>(s);
  }

} } //namespace gregorian

  
    
    




















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

