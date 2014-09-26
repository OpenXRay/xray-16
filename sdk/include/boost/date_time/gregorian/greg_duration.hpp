#ifndef GREG_DURATION_HPP___
#define GREG_DURATION_HPP___
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */

#include "boost/date_time/date_duration.hpp"
#include "boost/date_time/int_adapter.hpp"


namespace boost {
namespace gregorian {

  //!An internal date representation that includes infinities, not a date
  //typedef date_time::int_adapter<unsigned long> date_duration_rep;
  typedef long date_duration_rep;

  //! Durations in days for gregorian system
  /*! \ingroup date_basics
   */
  typedef date_time::date_duration<date_duration_rep> date_duration;
  //  typedef date_time::date_duration<long> date_duration;
  typedef date_duration days;

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
