
#ifndef DATE_TIME_TIME_SYSTEM_COUNTED_HPP
#define DATE_TIME_TIME_SYSTEM_COUNTED_HPP

/* Copyright (c) 2002 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland
 */


#include "boost/date_time/time_defs.hpp"
#include <string>


namespace boost {
namespace date_time {

  //! Time representation that uses a single integer count
  template<class config>
  struct counted_time_rep
  {
    typedef typename config::int_type   int_type;
    typedef typename config::date_type  date_type;
    typedef typename date_type::duration_type date_duration_type;
    typedef typename date_type::calendar_type calendar_type;
    typedef typename date_type::ymd_type ymd_type;
    typedef typename config::time_duration_type time_duration_type;
    typedef typename config::resolution_traits   resolution_traits;

    counted_time_rep(const date_type& d, const time_duration_type& tod) 
    {
      time_count_ = (d.day_number() * frac_sec_per_day()) + tod.ticks();
    }
    explicit counted_time_rep(int_type count) :
      time_count_(count)
    {}
    date_type date() const
    {
      typename calendar_type::date_int_type dc = day_count();
      //std::cout << "time_rep here:" << dc << std::endl;
      ymd_type ymd = calendar_type::from_day_number(dc);
      return date_type(ymd);
    }
    int_type day_count() const
    {
      return time_count_ / frac_sec_per_day();
    }
    int_type time_count() const
    {
      return time_count_;
    }
    int_type tod() const
    {
      return time_count_ % frac_sec_per_day();
    }
    static int_type frac_sec_per_day()
    {
      int_type seconds_per_day = 60*60*24;
      int_type fractional_sec_per_sec(resolution_traits::res_adjust());
      return seconds_per_day*fractional_sec_per_sec;
    }
  private:
    int_type time_count_;
  };

  //! An unadjusted time system implementation.
  template<class time_rep>
  class counted_time_system
  {
   public:
    typedef time_rep time_rep_type;
    typedef typename time_rep_type::time_duration_type time_duration_type;
    typedef typename time_duration_type::fractional_seconds_type fractional_seconds_type;
    typedef typename time_rep_type::date_type date_type;
    typedef typename time_rep_type::date_duration_type date_duration_type;

    static time_rep_type get_time_rep(const date_type& day,
                                      const time_duration_type& tod,
                                      date_time::dst_flags)
    {
      return time_rep_type(day, tod);
    }
    static date_type get_date(const time_rep_type& val)
    {
      return val.date();
    }
    static time_duration_type get_time_of_day(const time_rep_type& val)
    {
      return time_duration_type(0,0,0,val.tod()); 
    }
    static std::string zone_name(const time_rep_type&)
    {
      return "";
    }
    static bool is_equal(const time_rep_type& lhs, const time_rep_type& rhs)
    {
      return (lhs.time_count() == rhs.time_count());
    }
    static bool is_less(const time_rep_type& lhs, const time_rep_type& rhs)
    {
      return (lhs.time_count() < rhs.time_count());
    }
    static time_rep_type add_days(const time_rep_type& base,
                                  const date_duration_type& dd)
    {
      return time_rep_type(base.time_count() + (dd.days() * time_rep_type::frac_sec_per_day()));
    }
    static time_rep_type subtract_days(const time_rep_type& base,
                                       const date_duration_type& dd)
    {
      return time_rep_type(base.time_count() - (dd.days() * time_rep_type::frac_sec_per_day()));
    }
    static time_rep_type subtract_time_duration(const time_rep_type& base,
                                                const time_duration_type& td)
    {
      return time_rep_type(base.time_count() - td.ticks());
    }
    static time_rep_type add_time_duration(const time_rep_type& base,
                                           time_duration_type td)
    {
      return time_rep_type(base.time_count() + td.ticks());
    }
    static time_duration_type subtract_times(const time_rep_type& lhs,
                                             const time_rep_type& rhs)
    {
      fractional_seconds_type fs = lhs.time_count() - rhs.time_count();
      return time_duration_type(0,0,0,fs); 
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
 *
 */


#endif
