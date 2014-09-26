#ifndef ISO_FORMAT_HPP___
#define ISO_FORMAT_HPP___
/* Copyright (c) 2001 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland
 */

#include "boost/date_time/parse_format_base.hpp"

namespace boost {
namespace date_time {

//! Class to provide common iso formatting spec
class iso_format_base {
public:
  //! Describe month format -- its an integer in iso format
  static month_format_spec month_format()
  {
    return month_as_integer;
  }

  //! String used printed is date is invalid
  static const char* not_a_date()
  {       //20010102
    return "NotADate";
  }
  //! String used to for positive infinity value
  static const char* pos_infinity()
  {
    return "+infin  ";
  }
  //! String used to for positive infinity value
  static const char* neg_infinity()
  {
    return "-infin  ";
  }

  //! ISO char for a year -- used in durations
  static char year_sep_char()
  {
    return 'Y';
  }
  //! ISO char for a month
  static char month_sep_char()
  {
    return '-';
  }
  //! ISO char for a day
  static char day_sep_char()
  {
    return '-';
  }
  //! char for minute
  static char hour_sep_char()
  {
    return ':';
  }
  //! char for minute
  static char minute_sep_char()
  {
    return ':';
  }
  //! char for second
  static char second_sep_char()
  {
    return ':';
  }
  //! ISO char for a period
  static char period_start_char()
  {
    return 'P';
  }
  //! Used in time in mixed strings to set start of time
  static char time_start_char()
  {
    return 'T';
  }

  //! Used in mixed strings to identify start of a week number
  static char week_start_char()
  {
    return 'W';
  }

  //! Separators for periods
  static char period_sep_char()
  {
    return '/';
  }
  //! Separator for hh:mm:ss
  static char time_sep_char()
  {
    return ':';
  }
  //! Preferred Separator for hh:mm:ss,decimal_fraction
  static char fractional_time_sep_char()
  {
    return ',';
  }

  static bool is_component_sep(char sep)
  {
    switch(sep) {
    case 'H':
    case 'M':
    case 'S':
    case 'W':
    case 'T':
    case 'Y':
    case 'D':return true;
    default:
      return false;
    }
  }

  static bool is_fractional_time_sep(char sep)
  {
    switch(sep) {
    case ',':
    case '.': return true;
    default: return false;
    }
  }
  static bool is_timezone_sep(char sep)
  {
    switch(sep) {
    case '+':
    case '-': return true;
    default: return false;
    }
  }
  static char element_sep_char()
  {
    return '-';
  }

};


//! Format description for iso normal YYYYMMDD
class iso_format : public iso_format_base {
public:
  //! The ios standard format doesn't use char separators
  static bool has_date_sep_chars()
  {
    return false;
  }
};

//! Extended format uses seperators YYYY-MM-DD
class iso_extended_format : public iso_format_base {
public:
  //! Extended format needs char separators
  static bool has_date_sep_chars()
  {
    return true;
  }

};

} } //namespace date_time

/* Copyright (c) 2001
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
