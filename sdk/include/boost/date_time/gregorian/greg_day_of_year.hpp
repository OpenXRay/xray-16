#ifndef GREG_DAY_OF_YEAR_HPP___
#define GREG_DAY_OF_YEAR_HPP___
/* Copyright (c) 2000 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/constrained_value.hpp"
#include <stdexcept>
#include <string>

namespace boost {
namespace gregorian {

  //! Exception type for day of year (1..366)
  struct bad_day_of_year : public std::out_of_range
  {
    bad_day_of_year() : 
      std::out_of_range(std::string("Day of year value is out of range 1..366")) 
    {}
  };

  //! A day of the year range (1..366)
  typedef CV::simple_exception_policy<unsigned short,1,366,bad_day_of_year> greg_day_of_year_policies;

  //! Define a range representation type for the day of the year 1..366
  typedef CV::constrained_value<greg_day_of_year_policies> greg_day_of_year_rep;


} } //namespace gregorian

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
