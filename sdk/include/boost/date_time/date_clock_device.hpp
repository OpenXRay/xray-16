
#ifndef DATE_CLOCK_DEVICE_HPP___
#define DATE_CLOCK_DEVICE_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/c_time.hpp"


namespace boost {
namespace date_time {

  //! A clock providing day level services based on C time_t capabilities
  /*! This clock uses Posix interfaces as its implemenation and hence
   *  uses the timezone settings of the operating system.  Incorrect
   *  user settings will result in incorrect results for the calls
   *  to local_day.
   */
  template<class date_type> 
  class day_clock
  {
  public:
    typedef typename date_type::ymd_type ymd_type;
    //! Get the local day as a date type
    static date_type local_day() 
    {
      return date_type(local_day_ymd());
    }
    //! Get the local day as a ymd_type
    static typename date_type::ymd_type local_day_ymd() 
    {
      ::std::tm* curr = get_local_time();
      return ymd_type(curr->tm_year + 1900, 
                      curr->tm_mon + 1, 
                      curr->tm_mday);
    }
    //! Get the current day in universal date as a ymd_type
    static typename date_type::ymd_type universal_day_ymd() 
    {
      ::std::tm* curr = get_universal_time();
      return ymd_type(curr->tm_year + 1900, 
                      curr->tm_mon + 1, 
                      curr->tm_mday);
    }
    //! Get the UTC day as a date type
    static date_type universal_day() 
    {
      return date_type(universal_day_ymd());
    }

  private:
    static ::std::tm* get_local_time() 
    {
      ::std::time_t t;
      ::std::time(&t);
      return ::std::localtime(&t);
    }
    static ::std::tm* get_universal_time() 
    {
      ::std::time_t t;
      ::std::time(&t);
      return ::std::gmtime(&t);
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
