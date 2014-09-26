#ifndef DATE_TIME_DATE_HPP___
#define DATE_TIME_DATE_HPP___
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/year_month_day.hpp"
#include "boost/date_time/special_defs.hpp"
#include "boost/operators.hpp"

namespace boost {
namespace date_time {

  //!Representation of timepoint at the one day level resolution.
  /*! 
    The date template represents an interface shell for a date class
    that is based on a year-month-day system such as the gregorian
    or iso systems.  It provides basic operations to enable calculation
    and comparisons.  

    <b>Theory</b>

    This date representation fundamentally departs from the C tm struct 
    approach.  The goal for this type is to provide efficient date
    operations (add, subtract) and storage (minimize space to represent)
    in a concrete class.  Thus, the date uses a count internally to
    represent a particular date.  The calendar parameter defines 
    the policies for converting the the year-month-day and internal
    counted form here.  Applications that need to perform heavy
    formatting of the same date repeatedly will perform better
    by using the year-month-day representation.
    
    Internally the date uses a day number to represent the date.
    This is a monotonic time representation. This representation 
    allows for fast comparison as well as simplifying
    the creation of writing numeric operations.  Essentially, the 
    internal day number is like adjusted julian day.  The adjustment
    is determined by the Epoch date which is represented as day 1 of
    the calendar.  Day 0 is reserved for negative infinity so that
    any actual date is automatically greater than negative infinity.
    When a date is constructed from a date or formatted for output,
    the appropriate conversions are applied to create the year, month,
    day representations.
  */

  
  template<class T, class calendar, class duration_type_> 
  class date : private 
       boost::less_than_comparable<T 
     , boost::equality_comparable<T 
    > >
  {
  public:
    typedef T date_type;
    typedef calendar calendar_type;
    typedef typename calendar::date_traits_type traits_type;
    typedef duration_type_ duration_type;
    typedef typename calendar::year_type year_type;
    typedef typename calendar::month_type month_type;
    typedef typename calendar::day_type day_type;
    typedef typename calendar::ymd_type ymd_type;
    typedef typename calendar::date_rep_type date_rep_type;
    typedef typename calendar::date_int_type date_int_type;
    typedef typename calendar::day_of_week_type day_of_week_type;
    date(year_type year, month_type month, day_type day) 
      : days_(calendar::day_number(ymd_type(year, month, day)))
    {}
    date(const ymd_type& ymd) 
      : days_(calendar::day_number(ymd))
      {}
    //let the compiler write copy, assignment, and destructor
    year_type        year() const
    {
      ymd_type ymd = calendar::from_day_number(days_); 
      return ymd.year;
    }
    month_type       month() const
    {
      ymd_type ymd = calendar::from_day_number(days_); 
      return ymd.month;
    }
    day_type         day() const
    {
      ymd_type ymd = calendar::from_day_number(days_); 
      return ymd.day;
    }
    day_of_week_type day_of_week() const
    {
      ymd_type ymd = calendar::from_day_number(days_);
      return calendar::day_of_week(ymd);
    }
    ymd_type         year_month_day() const
    {
      return calendar::from_day_number(days_);
    }
    bool operator<(const date_type& rhs)  const
    {
      return days_ < rhs.days_;
    }
    bool operator==(const date_type& rhs) const
    {
      return days_ == rhs.days_;
    }
    //! check to see if date is not a value
    bool is_not_a_date()  const
    {
      return traits_type::is_not_a_number(days_);
    }
    //! check to see if date is one of the infinity values
    bool is_infinity()  const
    {
      return traits_type::is_inf(days_);
    }
    //! check to see if date is greater than all possible dates
    bool is_pos_infinity()  const
    {
      return traits_type::is_pos_infinity(days_);
    }
    //! check to see if date is greater than all possible dates
    bool is_neg_infinity()  const
    {
      return traits_type::is_neg_infinity(days_);
    }
    //! return as a special value or a not_special if a normal date
    special_values as_special()  const
    {
      return traits_type::to_special(days_);
    }
    duration_type operator-(const date_type& d) const
    {
      date_rep_type val = date_rep_type(days_) - date_rep_type(d.days_);
      return duration_type(val.as_number());
    }
    
    date_type operator-(const duration_type& dd) const
    {
      return date_type(date_rep_type(days_) - dd.days());
    }
    date_rep_type day_count() const {return days_;};
    //allow internal access from operators
    date_type operator+(const duration_type& dd) const
    {
      return date_type(date_rep_type(days_) + dd.days());
    }

    //see reference
  protected:
    /*! This is a private constructor which allows for the creation of new 
      dates.  It is not exposed to users since that would require class 
      users to understand the inner workings of the date class.
    */
    date(date_int_type days) : days_(days) {};
    date(date_rep_type days) : days_(days.as_number()) {};
    date_int_type days_;
    
  };



  
} } // namespace date_time

/* Copyright (c) 2000
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
