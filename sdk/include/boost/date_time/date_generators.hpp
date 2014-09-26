#ifndef DATE_TIME_DATE_GENERATORS_HPP__
#define DATE_TIME_DATE_GENERATORS_HPP__
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */
/*! @file date_generators.hpp
  Definition and implementation of date algorithm templates
*/

#include "boost/date_time/date.hpp"

namespace boost {
namespace date_time {

  
  //! Generates a date by applying the year to the given month and day.
  /*!
    Example usage: 
    @code
     partial_date pd(1, Jan);
     date d = pd.get_date(2002); //2002-Jan-01
    @endcode
    \ingroup date_alg
   */
  template<class date_type>
  class partial_date {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_type         day_type;
    typedef typename calendar_type::month_type       month_type;
    typedef typename calendar_type::year_type        year_type;
    typedef typename date_type::duration_type        duration_type;
    partial_date(day_type d, month_type m) :
      day_(d),
      month_(m)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type y) const
    {
      return date_type(y, month_, day_);
    }
    date_type operator()(year_type y) const
    {
      return date_type(y, month_, day_);
    }
    bool operator==(const partial_date& rhs) const
    {
      return (month_ == rhs.month_) && (day_ == rhs.day_);
    }
    bool operator<(const partial_date& rhs) const
    {
      if (month_ < rhs.month_) return true;
      if (month_ > rhs.month_) return false;
      //months are equal
      return (day_ < rhs.day_);
    }

    
  private:
    day_type day_;
    month_type month_;
  };


  //! Useful generator functor for finding holidays 
  /*! Based on the idea in Cal. Calc. for finding holidays that are
   *  the 'first Monday of September'.
   *  The algorithm here basically guesses for the first
   *  day of the month.  Then finds the first day of the correct
   *  type.  That is, if the first of the month is a Tuesday
   *  and it needs Wenesday then we simply increment by a day
   *  and then we can add the length of a week until we get
   *  to the 'nth kday'.  There are probably more efficient 
   *  algorithms based on using a mod 7, but this one works 
   *  reasonably well for basic applications.
   *  \ingroup date_alg
   */
  template<class date_type>
  class nth_kday_of_month {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type  day_of_week_type;
    typedef typename calendar_type::month_type        month_type;
    typedef typename calendar_type::year_type         year_type;
    typedef typename date_type::duration_type        duration_type;
    enum week_num {first=1, second, third, fourth, fifth};
    nth_kday_of_month(week_num week_no,
                      day_of_week_type dow,
                      month_type m) :
      month_(m),
      wn_(week_no),
      dow_(dow)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type y) const
    {
      date_type d(y, month_, 1); //first day of month
      duration_type one_day(1);
      duration_type one_week(7);
      while (dow_ != d.day_of_week()) {
        d = d + one_day;
      }
      int week = 1;
      while (week < wn_) {
        d = d + one_week;
        week++;
      }
      return d;
    }
  private:
    month_type month_;
    week_num wn_;
    day_of_week_type dow_;
  };

  //! Useful generator functor for finding holidays and daylight savings 
  /*! Similar to nth_kday_of_month, but requires less paramters
   *  \ingroup date_alg
   */
  template<class date_type>
  class first_kday_of_month {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type  day_of_week_type;
    typedef typename calendar_type::month_type        month_type;
    typedef typename calendar_type::year_type         year_type;
    typedef typename date_type::duration_type        duration_type;
    //!Specify the first 'Sunday' in 'April' spec
    /*!@param dow The day of week, eg: Sunday, Monday, etc
     * @param month The month of the year, eg: Jan, Feb, Mar, etc
     */ 
    first_kday_of_month(day_of_week_type dow, month_type month) :
      month_(month),
      dow_(dow)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type year) const
    {
      date_type d(year, month_,1);
      duration_type one_day(1);
      while (dow_ != d.day_of_week()) {
        d = d + one_day;
      }
      return d;
    }
  private:
    month_type month_;
    day_of_week_type dow_;
  };



  //! Calculate something like Last Sunday of January
  /*! Useful generator functor for finding holidays and daylight savings  
   *  Get the last day of the month and then calculate the difference
   *  to the last previous day.
   *  @param date_type A date class that exports day_of_week, month_type, etc.
   *  \ingroup date_alg
   */
  template<class date_type>
  class last_kday_of_month {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type  day_of_week_type;
    typedef typename calendar_type::month_type        month_type;
    typedef typename calendar_type::year_type         year_type;
    typedef typename date_type::duration_type        duration_type;
    //!Specify the date spec like last 'Sunday' in 'April' spec
    /*!@param dow The day of week, eg: Sunday, Monday, etc
     * @param month The month of the year, eg: Jan, Feb, Mar, etc
     */ 
    last_kday_of_month(day_of_week_type dow, month_type month) :
      month_(month),
      dow_(dow)
    {}
    //! Return a concrete date when provided with a year specific year.
    date_type get_date(year_type year) const
    {
      date_type d(year, month_, calendar_type::end_of_month_day(year,month_));
      duration_type one_day(1);
      while (dow_ != d.day_of_week()) {
        d = d - one_day;
      }
      return d;
    }
  private:
    month_type month_;
    day_of_week_type dow_;
  };


  //! Calculate something like "First Sunday after Jan 1,2002
  /*! Date generator that takes a date and finds kday after
   *@code
      typedef boost::date_time::first_kday_after<date> firstkdayafter;
      firstkdayafter fkaf(Monday);
      fkaf.get_date(date(2002,Feb,1));
    @endcode
   *  \ingroup date_alg
   */
  template<class date_type>
  class first_kday_after {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type day_of_week_type;
    typedef typename date_type::duration_type        duration_type;
    first_kday_after(day_of_week_type dow) :
      dow_(dow)
    {}
    //! Return next kday given.
    date_type get_date(date_type start_day) const
    {
      duration_type one_day(1);
      date_type d = start_day + one_day;
      while (dow_ != d.day_of_week()) {
        d = d + one_day;
      }
      return d;
    }
    
  private:
    day_of_week_type dow_;
  };

  //! Calculate something like "First Sunday before Jan 1,2002
  /*! Date generator that takes a date and finds kday after
   *@code
      typedef boost::date_time::first_kday_before<date> firstkdaybefore;
      firstkdaybefore fkbf(Monday);
      fkbf.get_date(date(2002,Feb,1));
    @endcode
   *  \ingroup date_alg
   */
  template<class date_type>
  class first_kday_before {
  public:
    typedef typename date_type::calendar_type calendar_type;
    typedef typename calendar_type::day_of_week_type day_of_week_type;
    typedef typename date_type::duration_type        duration_type;
    first_kday_before(day_of_week_type dow) :
      dow_(dow)
    {}
    //! Return next kday given.
    date_type get_date(date_type start_day) const
    {
      duration_type one_day(1);
      date_type d = start_day - one_day;
      while (dow_ != d.day_of_week()) {
        d = d - one_day;
      }
      return d;
    }
    
  private:
    day_of_week_type dow_;
  };


} } //namespace date_time


/*
 * Copyright (c) 2001
 * CrystalClear Software, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  CrystalClear Software makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided as is without express or implied warranty.
 *
 *
 * Author:  Jeff Garland (jeff@CrystalClearSoftware.com)
 * Created: Sun Sep  9 17:20:39 2001 
 *
 */


#endif
