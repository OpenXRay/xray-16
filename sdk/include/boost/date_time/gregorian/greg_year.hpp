#ifndef GREG_YEAR_HPP___
#define GREG_YEAR_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/constrained_value.hpp"
#include <stdexcept>
#include <string>

namespace boost {
namespace gregorian {

  //! Exception type for gregorian year
  struct bad_year : public std::out_of_range
  {
    bad_year() : 
      std::out_of_range(std::string("Year is out of valid range: 1400..10000")) 
    {}
  };
  //! Policy class that declares error handling gregorian year type
  typedef CV::simple_exception_policy<unsigned short, 1400, 10000, bad_year> greg_year_policies;

  //! Generated representation for gregorian year
  typedef CV::constrained_value<greg_year_policies> greg_year_rep;

  //! Represent a day of the month (range 1900 - 10000) 
  /*! This small class allows for simple conversion an integer value into
      a year for the gregorian calendar.  This currently only allows a
      range of 1900 to 10000.  Both ends of the range are a bit arbitrary
      at the moment, but they are the limits of current testing of the 
      library.  As such they may be increased in the future.
  */
  class greg_year : public greg_year_rep {
  public:
    greg_year(unsigned short year) : greg_year_rep(year) {}
    operator unsigned short()  const {return value_;}
  private:
    
  };



} } //namespace gregorian

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
