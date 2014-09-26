#ifndef DATE_TIME_SIMPLE_FORMAT_HPP___
#define DATE_TIME_SIMPLE_FORMAT_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland
 */

#include "boost/date_time/parse_format_base.hpp"

namespace boost {
namespace date_time {

//! Class to provide simple basic formatting rules
class simple_format {
public:

  //! String used printed is date is invalid
  static const char* not_a_date()
  {
    return "not-a-date";
  }
  //! String used to for positive infinity value
  static const char* pos_infinity()
  {       //2001-Jan-03
    return "+infinity  ";
  }
  //! String used to for positive infinity value
  static const char* neg_infinity()
  {
    return "-infinity  ";
  }
  //! Describe month format
  static month_format_spec month_format()
  {
    return month_as_short_string;
  }
  static ymd_order_spec date_order()
  {
    return ymd_order_iso; //YYYY-MM-DD
  }
  //! This format uses '-' to separate date elements
  static bool has_date_sep_chars()
  {
    return true;
  }
  //! Char to sep?
  static char year_sep_char()
  {
    return '-';
  }
  //! char between year-month
  static char month_sep_char()
  {
    return '-';
  }
  //! Char to separate month-day
  static char day_sep_char()
  {
    return '-';
  }
  //! char between date-hours
  static char hour_sep_char()
  {
    return ' ';
  }
  //! char between hour and minute
  static char minute_sep_char()
  {
    return ':';
  }
  //! char for second
  static char second_sep_char()
  {
    return ':';
  }

};

} } //namespace date_time

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
