#ifndef GREGORIAN_GREGORIAN_CALENDAR_HPP__
#define GREGORIAN_GREGORIAN_CALENDAR_HPP__
/* Copyright (c) 2001 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/gregorian/greg_weekday.hpp"
#include "boost/date_time/gregorian/greg_day_of_year.hpp"
#include "boost/date_time/gregorian_calendar.hpp"
#include "boost/date_time/gregorian/greg_ymd.hpp"
#include "boost/date_time/int_adapter.hpp"

namespace boost {
namespace gregorian {
    
  //!An internal date representation that includes infinities, not a date
  typedef date_time::int_adapter<unsigned long> fancy_date_rep;

  //! Gregorian calendar for this implementation, hard work in the base
  class gregorian_calendar : 
    public date_time::gregorian_calendar_base<greg_year_month_day, fancy_date_rep::int_type> {
  public:
    //! Type to hold a weekday (eg: Sunday, Monday,...)
    typedef greg_weekday         day_of_week_type;
    //! Counter type from 1 to 366 for gregorian dates.
    typedef greg_day_of_year_rep day_of_year_type;
    //! Internal date representation that handles infinity, not a date
    typedef fancy_date_rep       date_rep_type;
    //! Date rep implements the traits stuff as well
    typedef fancy_date_rep       date_traits_type;

    
  private:
  };

} } //namespace gregorian
  

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
  
