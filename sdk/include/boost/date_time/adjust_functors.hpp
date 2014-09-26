#ifndef _DATE_TIME_ADJUST_FUNCTORS_HPP___
#define _DATE_TIME_ADJUST_FUNCTORS_HPP___
/* Copyright (c) 2001 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/date.hpp"
#include "boost/date_time/wrapping_int.hpp"

namespace boost {
namespace date_time {
  

  //! Functor to iterate a fixed number of days
  template<class date_type>
  class day_functor 
  {
  public:
    typedef typename date_type::duration_type duration_type;
    day_functor(int f) : f_(f) {}
    duration_type get_offset(const date_type& d) const 
    {
      return duration_type(f_);
    }
  private:
    int f_;
  };


  //! Provides calculation to find next nth month given a date
  /*! This adjustment function provides the logic for 'month-based'
   *  advancement on a ymd based calendar.  The policy it uses 
   *  to handle the non existant end of month days is to back
   *  up to the last day of the month.  Also, if the starting
   *  date is the last day of a month, this functor will attempt
   *  to adjust to the end of the month.

   */
  template<class date_type>
  class month_functor 
  {
  public:
    typedef typename date_type::duration_type duration_type;
    typedef typename date_type::calendar_type cal_type;
    typedef typename cal_type::ymd_type ymd_type;
    typedef typename cal_type::day_type day_type;

    month_functor(int f) : f_(f), origDayOfMonth_(0) {}
    duration_type get_offset(const date_type& d) const 
    {
      ymd_type ymd(d.year_month_day());
      if (origDayOfMonth_ == 0) {
        origDayOfMonth_ = ymd.day;
        day_type endOfMonthDay(cal_type::end_of_month_day(ymd.year,ymd.month));
        if (endOfMonthDay == ymd.day) {
          origDayOfMonth_ = -1; //force the value to the end of month
        }
      }
      date_time::wrapping_int2<short,1,12> wi(ymd.month);
      unsigned long year = wi.add(f_); //calc the year wrap around
      year += ymd.year; //now add in the current year
//       std::cout << "trace wi: " << wi.as_int() << std::endl;
//       std::cout << "trace year: " << year << std::endl;
      //find the last day for the new month
      day_type resultingEndOfMonthDay(cal_type::end_of_month_day(year, wi.as_int()));
      //original was the end of month -- force to last day of month
      if (origDayOfMonth_ == -1) {
        return date_type(year, wi.as_int(), resultingEndOfMonthDay) - d;
      }
      day_type dayOfMonth = origDayOfMonth_;
      if (dayOfMonth > resultingEndOfMonthDay) {
        dayOfMonth = resultingEndOfMonthDay;
      }
      return date_type(year, wi.as_int(), dayOfMonth) - d;
    }
  private:
    int f_;
    mutable short origDayOfMonth_;
  };


  //! Functor to iterate a over weeks
  template<class date_type>
  class week_functor 
  {
  public:
    typedef typename date_type::duration_type duration_type;
    typedef typename date_type::calendar_type calendar_type;
    week_functor(int f) : f_(f) {}
    duration_type get_offset(const date_type& d) const 
    {
      return duration_type(f_*calendar_type::days_in_week());
    }
  private:
    int f_;
  };

  //! Functor to iterate by a year adjusting for leap years
  /*!
   *@throws bad_day if date value is invalid (eg: feb 29) 
   */
  template<class date_type>
  class year_functor 
  {
  public:
    typedef typename date_type::year_type year_type;
    typedef typename date_type::duration_type duration_type;
    year_functor(int f) : f_(f) {}
    duration_type get_offset(const date_type& d) const 
    {
      date_type new_date(d.year()+f_, d.month(), d.day());
      return new_date-d;
    }
  private:
    int f_;
  };
  

  
} }//namespace date_time

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
