#ifndef DATE_TIME_TIME_ITERATOR_HPP___
#define DATE_TIME_TIME_ITERATOR_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */


namespace boost {
namespace date_time {
  

  //! Simple time iterator skeleton class
  template<class time_type>
  class time_itr {
  public:
    typedef typename time_type::time_duration_type time_duration_type;
    time_itr(time_type t, time_duration_type d) : current_(t), offset_(d) {};
    time_itr& operator++() 
    {
      current_ = current_ + offset_;
      return *this;
    };
    time_type operator*() {return current_;};
    time_type* operator->() {return &current_;};
    bool operator<  (const time_type& t) {return current_ < t;};
    bool operator<= (const time_type& t) {return current_ <= t;};
    bool operator!=  (const time_type& t) {return current_ != t;};
    bool operator== (const time_type& t) {return current_ == t;};
    bool operator>  (const time_type& t) {return current_ > t;};
    bool operator>= (const time_type& t) {return current_ >= t;};
    
  private:
    time_type current_;
    time_duration_type offset_;
  };
  

  
} }//namespace date_time

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
