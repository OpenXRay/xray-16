#ifndef DATE_TIME_PERIOD_HPP___
#define DATE_TIME_PERIOD_HPP___
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

/*! \file period.hpp
  This file contain the implementation of the period abstraction. This is
  basically the same idea as a range.  Although this class is intended for
  use in the time library, it is pretty close to general enough for other
  numeric uses.

*/

#include "boost/operators.hpp"


namespace boost {
namespace date_time {
  //!Provides generalized period type useful in date-time systems
  /*!This template uses a class to represent a time point within the period
    and another class to represent a duration.  As a result, this class is
    not appropriate for use when the number and duration representation 
    are the same (eg: in the regular number domain).
    
    A period can be specified by providing either the starting point and 
    a duration or the starting point and the last point.  A period
    will always have a duration of at least 1 and it's start will always
    be before or eqaul to the last.  

    In the case that the begin and last are the same, the period has a 
    length of one unit.  For example, suppose this is a period of days.
    That is, each "point" represents a single day.  If the start and the
    last is the same day then the period represents that single day for
    a length of one.  The same applies if each "point" represents a month
    or a year.  The way to think of this is that the granularity of the 
    point_rep class is similar to the ticks on a ruler.  The more ticks,
    the finer the resolution of a range that can be defined. A range 
    defined on a ruler with 1cm resolution between the 1cm mark and the 
    2cm mark is 1cm long.  In the ruler range, the 1cm mark is in the
    range while the 2cm mark is not.  

    While the ruler analogy useful, it is not how date ranges are naturally 
    thought about (at least by me).  That is, it is more natural to think 
    of a date as including up to the end of the second time point.  So when
    I say day 1 to day 2 I usually mean from the beginning of day 1 to the
    end of day 2.  

    The best way to handle periods is usually to provide a start point and
    a duration.  So, day1 + 7 days is a week period which includes all of the
    first day and 6 more days (eg: Sun to Sat).

   */
  template<class point_rep, class duration_rep>
  class period : private
      boost::less_than_comparable<period<point_rep, duration_rep> 
    , boost::equality_comparable< period<point_rep, duration_rep> 
    > >
  {
  public:
    typedef point_rep point_type;
    typedef duration_rep duration_type;

    period(point_rep begin, point_rep last);
    period(point_rep begin, duration_rep len);
    point_rep begin() const;
    point_rep end() const;
    point_rep last() const;
    duration_rep length() const;
    bool is_null() const;
    bool operator==(const period& rhs) const;
    bool operator<(const period& rhs) const;
    void shift(const duration_rep& d);
    bool contains(const point_rep& point) const;
    bool contains(const period& other) const;
    bool intersects(const period& other) const;
    bool is_adjacent(const period& other) const;
    bool is_before(const point_rep& point) const;
    bool is_after(const point_rep& point) const;
    period intersection(const period& other) const;
    period merge(const period& other) const;
  private:
    point_rep begin_;
    point_rep last_;
  };

  //! create a period from begin to last eg: [begin,end)
  /*! If last <= begin then the period will is defined as null
   */
  template<class point_rep, class duration_rep>
  inline
  period<point_rep,duration_rep>::period(point_rep begin, 
                                         point_rep end) : 
    begin_(begin), 
    last_(end - duration_rep::unit())
  {}

  //! create a period as [begin, begin+len)
  /*! If len is <= 0 then the period will be defined as null
   */
  template<class point_rep, class duration_rep>
  inline
  period<point_rep,duration_rep>::period(point_rep begin, duration_rep len) :
    begin_(begin), 
    last_(begin + len-duration_rep::unit()) 
  {}


  //! Return the first element in the period
  template<class point_rep, class duration_rep>
  inline
  point_rep period<point_rep,duration_rep>::begin() const 
  {
    return begin_;
  }

  //! Return one past the last element 
  template<class point_rep, class duration_rep>
  inline
  point_rep period<point_rep,duration_rep>::end() const 
  {
    return  last_ + duration_rep::unit(); 
  }

  //! Return the last item in the period
  template<class point_rep, class duration_rep>
  inline
  point_rep period<point_rep,duration_rep>::last() const 
  {
    return last_;
  }

  //! True if period is ill formed
  template<class point_rep, class duration_rep>
  inline
  bool period<point_rep,duration_rep>::is_null() const 
  {
    return last_ <= begin_;
  }

  //! Return the length of the period
  template<class point_rep, class duration_rep>
  inline
  duration_rep period<point_rep,duration_rep>::length() const
  {
    return end() - begin_;
  }

  //! Equality operator
  template<class point_rep, class duration_rep>
  inline
  bool period<point_rep,duration_rep>::operator==(const period& rhs) const 
  {
    return  ((begin_ == rhs.begin_) && 
             (last_ == rhs.last_));
  }

  //! Strict as defined by rhs.last <= lhs.last
  template<class point_rep, class duration_rep>
  inline
  bool period<point_rep,duration_rep>::operator<(const period& rhs) const 
  {
    return (last_ <= rhs.begin_);
  } 


  //! Shift the start and end by the specified amount
  template<class point_rep, class duration_rep>
  inline
  void period<point_rep,duration_rep>::shift(const duration_rep& d)
  {
    begin_ = begin_ + d;
    last_  = last_  + d;
  }

  //! True if the point is inside the period
  template<class point_rep, class duration_rep>
  inline
  bool period<point_rep,duration_rep>::contains(const point_rep& point) const 
  {
    return ((point >= begin_) &&
            (point <= last_));
  }


  //! True if this period fully contains (or equals) the other period
  template<class point_rep, class duration_rep>
  inline
  bool period<point_rep,duration_rep>::contains(const period<point_rep,duration_rep>& other) const
  {
    return ((begin_ <= other.begin_) && (last_ >= other.last_));
  }


  //! True if periods are next to each other without a gap.
  /* In the example below, p1 and p2 are adjacent, but p3 is not adjacent
   * with either of p1 or p2.
   *@code
   *   [-p1-)
   *        [-p2-)
   *          [-p3-) 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline
  bool 
  period<point_rep,duration_rep>::is_adjacent(const period<point_rep,duration_rep>& other) const 
  { 
    return (other.begin() == end() ||
            begin_ == other.end());
  }


  //! True if all of the period is prior or t < start
  /* In the example below only point 1 would evaluate to true.
   *@code
   *     [---------])
   * ^   ^    ^     ^   ^
   * 1   2    3     4   5
   * 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline
  bool 
  period<point_rep,duration_rep>::is_after(const point_rep& t) const 
  { 
    if (is_null()) 
    {
      return false; //null period isn't after
    }
    
    return t < begin_;
  }

  //! True if all of the period is prior to the passed point or end <= t
  /* In the example below points 4 and 5 return true.
   *@code
   *     [---------])
   * ^   ^    ^     ^   ^
   * 1   2    3     4   5
   * 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline
  bool 
  period<point_rep,duration_rep>::is_before(const point_rep& t) const 
  { 
    if (is_null()) 
    {
      return false;  //null period isn't before anything
    }
    
    return last_ < t;
  }


  //! True if the periods overlap in any way
  /* In the example below p1 intersects with p2, p4, and p6.
   *@code
   *       [---p1---)
   *             [---p2---)
   *                [---p3---) 
   *  [---p4---) 
   * [-p5-) 
   *         [-p6-) 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline
  bool period<point_rep,duration_rep>::intersects(const period<point_rep,duration_rep>& other) const 
  { 
    return ( contains(other.begin_) ||
             other.contains(begin_) ||
             ((other.begin_ < begin_) && (other.last_ >= begin_)));
  }

  //! Returns the period of intersection or invalid range no intersection
  template<class point_rep, class duration_rep>
  inline
  period<point_rep,duration_rep>
  period<point_rep,duration_rep>::intersection(const period<point_rep,duration_rep>& other) const 
  {
    if (begin_ > other.begin_) {
      if (last_ <= other.last_) { //case2
        return *this;  
      }
      //case 1
      return period<point_rep,duration_rep>(begin_, other.end());
    }
    else {
      if (last_ <= other.last_) { //case3
        return period<point_rep,duration_rep>(other.begin_, this->end());
      }
      //case4
      return other;
    }
    //unreachable
  }

  //! Returns the union of intersecting periods -- or null period
  /*! 
   */
  template<class point_rep, class duration_rep>
  inline
  period<point_rep,duration_rep>
  period<point_rep,duration_rep>::merge(const period<point_rep,duration_rep>& other) const 
  {
    if (this->intersects(other)) {      
      if (begin_ < other.begin_) {
        return period<point_rep,duration_rep>(begin_, last_ > other.last_ ? this->end() : other.end());
      }
      
      return period<point_rep,duration_rep>(other.begin_, last_ > other.last_ ? this->end() : other.end());
      
    }
    return period<point_rep,duration_rep>(begin_,begin_); // no intersect return null
  }


} } //namespace date_time

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
