#ifndef POSIX_PTIME_HPP___
#define POSIX_PTIME_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/posix_time/posix_time_system.hpp"
#include "boost/date_time/time.hpp"

namespace boost {

namespace posix_time {
  
  //! Time type with no timezone or other adjustments
  /*! \ingroup time_basics
   */
  class ptime : public date_time::base_time<ptime, posix_time_system>
  {
  public:
    typedef posix_time_system time_system_type;
    typedef time_system_type::time_rep_type time_rep_type;
    typedef time_system_type::time_duration_type time_duration_type;
    typedef ptime time_type;
    //! Construct with date and offset in day
    ptime(gregorian::date d,time_duration_type td) : date_time::base_time<time_type,time_system_type>(d,td)
    {}
    //! Construct a time at start of the given day (midnight)
    explicit ptime(gregorian::date d) : date_time::base_time<time_type,time_system_type>(d,time_duration_type(0,0,0))
    {}
    //! Copy from time_rep
    ptime(const time_rep_type& rhs):
      date_time::base_time<time_type,time_system_type>(rhs)
    {}
  };



} }//namespace posix_time

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
 *
 */

#endif

