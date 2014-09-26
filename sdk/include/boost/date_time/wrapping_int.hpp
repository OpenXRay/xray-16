#ifndef _DATE_TIME_WRAPPING_INT_HPP__
#define _DATE_TIME_WRAPPING_INT_HPP__
/* Copyright (c) 2001 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */


namespace boost {
namespace date_time {

//! A wrapping integer used to support time durations 
/*! In composite date and time types this type is used to
 *  wrap at the day boundary.
 *
 */
template<typename int_type_, int_type_ wrap_val>
class wrapping_int {
public:
  typedef int_type_ int_type;
  //typedef overflow_type_ overflow_type;
  static int_type wrap_value() {return wrap_val;}
  //!Add, return true if wrapped
  wrapping_int(int_type v) : value_(v) {};
  //! Explicit converion method
  int_type as_int()   const   {return value_;}
  operator int_type() const   {return value_;}
  int_type add(int_type v) 
  {
    //take the mod here and assign it....
    int_type remainder = v % wrap_val;
    int_type overflow = v / wrap_val;
    value_ += remainder;
    if ((value_) >= wrap_val) {
      value_ -= wrap_val;
      overflow++;
    }
    return overflow;
  }
  int_type subtract(int_type v) 
  {
    //take the mod here and assign it....
    int_type remainder = v % wrap_val;
    int_type underflow = v / wrap_val;
    
//     std::cout << "wi :" << value_ << "|"
//               << v << "|"
//               << underflow << "|" 
//               << remainder << "|" 
//               << wrap_val  << std::endl;
    if (remainder > value_) {
      underflow++;
      //      value_ = remainder - value_;
      value_ = wrap_val - (remainder-value_);
    }
    else {
      value_ -= remainder;
      //value_ = wrap_val-(remainder-value_);
      //      value_ = wrap_val -(value_-remainder);
    }
//     std::cout << "wi final uf: " << underflow 
//               << " value: "  << value_ << std::endl;
    return underflow;
  }
		    
private:
  int_type value_;
			      
};


//! A wrapping integer used to wrap around at the top
/*! Bad name, quick impl to fix a bug -- fix later!!
 *  This allows the wrap to restart at a value other than 0.
 *  Currently this only works if wrap_min == 1 
 */
template<typename int_type_, int_type_ wrap_min, int_type_ wrap_max>
class wrapping_int2 {
public:
  typedef int_type_ int_type;
  static unsigned long wrap_value() {return wrap_max;}
  static unsigned long min_value()  {return wrap_min;}
  //!Add, return true if wrapped
  wrapping_int2(int_type v) : value_(v) {};
  //! Explicit converion method
  int_type as_int()   const   {return value_;}
  operator int_type() const {return value_;}
  int_type add(int_type v) 
  {
    //take the mod here and assign it....
    int_type remainder = v % wrap_max;
    int_type overflow = v / wrap_max;
//     std::cout << "wi2: " << value_ << "|"
// 	      << v << "|"
// 	      << remainder << "|" 
// 	      << wrap_max  << "|"
//               << wrap_min  << std::endl;
    value_ += remainder;
    if ((value_) > wrap_max) {
      overflow++;
      value_ -= (wrap_max - wrap_min + 1);
    }
//     std::cout << "wi2 final: " 
// 	      << overflow << "|" 
// 	      << value_ << std::endl;
    return overflow;
  }
		    
private:
  int_type value_;
			      
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
 * Created: Sat Sep  8 19:37:11 2001 
 *
 */


#endif
