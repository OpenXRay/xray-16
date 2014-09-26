#ifndef DATE_ITERATOR_HPP___
#define DATE_ITERATOR_HPP___
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include <iterator>

namespace boost {
namespace date_time {
  //! An iterator over dates with varying resolution (day, week, month, year, etc)
  enum date_resolutions {day, week, months, year, decade, century, NumDateResolutions};

  //! Base date iterator type
  /*! This class provides the skeleton for the creation of iterators.
   *  New and interesting interators can be created by plugging in a new
   *  function that derives the next value from the current state.
   *  generation of various types of -based information.
   *
   *  <b>Template Parameters</b>
   *
   *  <b>date_type</b>
   *
   *  The date_type is a concrete date_type. The date_type must
   *  define a duration_type and a calendar_type.
   */
  template<class date_type>
  class date_itr_base {
  // works, but benefit unclear at the moment
  //   class date_itr_base : public std::iterator<std::input_iterator_tag, 
  //                                             date_type, void, void, void>{
  public:
    typedef typename date_type::duration_type duration_type;
    typedef date_type value_type;
    typedef std::input_iterator_tag iterator_category;

    date_itr_base(date_type d) : current_(d) {}
    virtual ~date_itr_base() {};
    date_itr_base& operator++() 
    {
      current_ = current_ + get_offset(current_);
      return *this;
    };
    virtual duration_type get_offset(const date_type& current) const=0;
    date_type operator*() {return current_;};
    date_type* operator->() {return &current_;};
    bool operator<  (const date_type& d) {return current_ < d;}
    bool operator<= (const date_type& d) {return current_ <= d;}
    bool operator>  (const date_type& d) {return current_ > d;}
    bool operator>= (const date_type& d) {return current_ >= d;}
    bool operator== (const date_type& d) {return current_ == d;}
    bool operator!= (const date_type& d) {return current_ != d;}    
  private:
    date_type current_;
  };
  
  //! Overrides the base date iterator providing hook for functors
  /*
   *  <b>offset_functor</b>
   *
   *  The offset functor must define a get_offset function that takes the
   *  current point in time and calculates and offset.
   *
   */
  template<class offset_functor, class date_type>
  class date_itr : public date_itr_base<date_type> {
  public:
    typedef typename date_type::duration_type duration_type;
    date_itr(date_type d, int factor=1) : 
      date_itr_base<date_type>(d), 
      of_(factor) 
    {}
    virtual duration_type get_offset(const date_type& current) const
    {
      return of_.get_offset(current);
    }
  private:
    offset_functor of_;
  };
  

  
} } //namespace date_time

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
