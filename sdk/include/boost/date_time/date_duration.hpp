#ifndef DATE_TIME_DATE_DURATION__
#define DATE_TIME_DATE_DURATION__
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */


#include <boost/operators.hpp>

namespace boost {
namespace date_time {
  

  //! Duration type with date level resolution
  template<class duration_rep>
  class date_duration : private
      boost::less_than_comparable<date_duration< duration_rep> 
    , boost::equality_comparable< date_duration< duration_rep> 
    > >
  {
  public:
    //! Construct from a day count
    explicit date_duration(duration_rep days) : days_(days) {};
    duration_rep days() const 
    {
      return days_;
    };
    //! Returns the smallest duration -- used by to calculate 'end'
    static date_duration unit()
    {
      return date_duration<duration_rep>(1);
    }
    //! Equality 
    bool operator==(const date_duration& rhs) const 
    {
      return days_ == rhs.days_;
    }
    //! Less 
    bool operator<(const date_duration& rhs) const 
    {
      return days_ < rhs.days_;
    }
    //! Subtract another duration -- result is signed
    date_duration operator-(const date_duration& rhs) const
    {
      return date_duration<duration_rep>(days_ - rhs.days_);
    }
    //! Add a duration -- result is signed
    date_duration operator+(const date_duration& rhs) const
    {
      return date_duration<duration_rep>(days_ + rhs.days_);
    }
    //! return sign information
    bool is_negative() const
    {
      return days_ < 0;
    }
  private:
    duration_rep days_;
  };  

} } //namspace date_time

/* Copyright (c) 2000, 2001
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
