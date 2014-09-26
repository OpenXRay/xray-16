#ifndef _GREGORIAN_TYPES_HPP__
#define _GREGORIAN_TYPES_HPP__
/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland 
 */
/*! @file gregorian_types.hpp
  Single file header that defines most of the types for the gregorian 
  date-time system.
*/


#include "boost/date_time/date.hpp"
#include "boost/date_time/period.hpp"
#include "boost/date_time/gregorian/greg_calendar.hpp"
#include "boost/date_time/gregorian/greg_duration.hpp"
#include "boost/date_time/gregorian/greg_date.hpp"
#include "boost/date_time/date_generators.hpp"
#include "boost/date_time/date_clock_device.hpp"
#include "boost/date_time/date_iterator.hpp"
#include "boost/date_time/adjust_functors.hpp"

namespace boost {

//! Gregorian date system based on date_time components
/*! This date system defines a full complement of types including
 *  a date, date_duration, date_period, day_clock, and a
 *  day_iterator.
 */
namespace gregorian {
  //! Date periods for the gregorian system
  /*!\ingroup date_basics
   */
  typedef date_time::period<date, date_duration> date_period;  
  //! A date generation object type
  typedef date_time::partial_date<date> partial_date;
  //! A clock to get the current day from the local computer
  /*!\ingroup date_basics
   */
  typedef date_time::day_clock<date> day_clock;

  //! Base date_iterator type for gregorian types.
  /*!\ingroup date_basics
   */
  typedef date_time::date_itr_base<date> date_iterator;

  //! A day level iterator
  /*!\ingroup date_basics
   */
  typedef date_time::date_itr<date_time::day_functor<date>,
                              date> day_iterator;
  //! A week level iterator
  /*!\ingroup date_basics
   */
  typedef date_time::date_itr<date_time::week_functor<date>,
                              date> week_iterator;
  //! A month level iterator
  /*!\ingroup date_basics
   */
  typedef date_time::date_itr<date_time::month_functor<date>,
                              date> month_iterator;
  //! A year level iterator
  /*!\ingroup date_basics
   */
  typedef date_time::date_itr<date_time::year_functor<date>,
                              date> year_iterator;
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
