#ifndef DATE_TIME_TIME_CLOCK_HPP___
#define DATE_TIME_TIME_CLOCK_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

/*! @file time_clock.hpp
  This file contains the interface for clock devices.  
*/

#include "boost/date_time/c_time.hpp"

namespace boost {
namespace date_time {


  //! A clock providing time level services based on C time_t capabilities
  /*! This clock provides resolution to the 1 second level
   */
  template<class date_type, class time_type> 
  class second_clock
  {
  public:
    //    typedef typename time_type::date_type date_type;
    typedef typename time_type::time_duration_type time_duration_type;

    static time_type local_time() 
    {
      ::std::time_t t;
      ::std::time(&t); 
      ::std::tm* curr = ::std::localtime(&t);
      return create_time(curr);
    }

    //! Get the current day in universal date as a ymd_type
    static time_type universal_time() 
    {

      ::std::time_t t;
      ::std::time(&t);
      ::std::tm* curr= ::std::gmtime(&t);
      return create_time(curr);
    }

  private:
    static time_type create_time(::std::tm* current)
    {
      date_type d(current->tm_year + 1900, 
                  current->tm_mon + 1, 
                  current->tm_mday);
      time_duration_type td(current->tm_hour,
                            current->tm_min,
                            current->tm_sec);
      return time_type(d,td);
    }
    
  };


} } //namespace date_time

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
