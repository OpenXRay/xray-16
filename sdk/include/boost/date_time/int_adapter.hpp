#ifndef _DATE_TIME_INT_ADAPTER_HPP__
#define _DATE_TIME_INT_ADAPTER_HPP__
/* Copyright (c) 2001 CrystalClear Software, Inc.
 * Disclaimer & Full Copyright at end of file
 * Author: Jeff Garland
 */


#include "boost/limits.hpp" //work around compilers without limits
#include "boost/date_time/special_defs.hpp"

namespace boost {
namespace date_time {

//! Adapter to create integer types with +-infinity, and not a value
/*! This class is used internally in counted date/time representations.
 *  It adds the floating point like features of infinities and
 *  not a number.
 */

template<typename int_type_>
class int_adapter {
public:
  typedef int_type_ int_type;
  int_adapter(int_type v) :
    value_(v)
  {}
  static bool has_infinity()
  {
    return  true;
  }
  static const int_adapter  pos_infinity()
  {
    return ::std::numeric_limits<int_type>::max();
  }
  static const int_adapter  neg_infinity()
  {
    return ::std::numeric_limits<int_type>::min();
  }
  static const int_adapter  not_a_number()
  {
    return ::std::numeric_limits<int_type>::max()-1;
  }
  static  int_adapter max()
  {
    return ::std::numeric_limits<int_type>::max()-2;
  }
  static  int_adapter min()
  {
    return ::std::numeric_limits<int_type>::min()+1;
  }
  static int_adapter from_special(special_values sv)
  {
    switch (sv) {
    case not_a_date_time: return not_a_number();
    case neg_infin:       return neg_infinity();
    case pos_infin:       return pos_infinity();
    case max_date_time:   return max();
    case min_date_time:   return min();
    default:              return not_a_number();
    }

  }
  static bool is_inf(int_type v)
  {
    return (v == neg_infinity().as_number() ||
            v == pos_infinity().as_number());
  }
  static bool is_neg_infinity(int_type v)
  {
    return (v == neg_infinity().as_number());
  }
  static bool is_pos_infinity(int_type v)
  {
    return (v == pos_infinity().as_number());
  }
  static bool is_not_a_number(int_type v)
  {
    return (v == not_a_number().as_number());
  }
  //! Returns either special value type or is_not_special
  static special_values to_special(int_type v)
  {
    if (is_not_a_number(v)) return not_a_date_time;
    if (is_neg_infinity(v)) return neg_infin;
    if (is_pos_infinity(v)) return pos_infin;
    return not_special;
  }


  //-3 leaves room for representations of infinity and not a date
  static  int_type maxcount()
  {
    return ::std::numeric_limits<int_type>::max()-3;
  }
  bool is_infinity() const
  {
    return (value_ == neg_infinity().as_number() ||
            value_ == pos_infinity().as_number());
  }
  bool is_nan() const
  {
    return (value_ == not_a_number().as_number());
  }
  bool operator==(const int_adapter& rhs) const
  {
    return value_ == rhs.value_;
  }
  bool operator!=(const int_adapter& rhs) const
  {
    return value_ != rhs.value_;
  }
  bool operator<(const int_adapter& rhs) const
  {
    return value_ < rhs.value_;
  }
  bool operator>(const int_adapter& rhs) const
  {
    return value_ > rhs.value_;
  }
  int_type as_number() const
  {
    return value_;
  }
  //! Returns either special value type or is_not_special
  special_values as_special() const
  {
    return int_adapter::to_special(value_);
  }
  //creates nasty ambiguities
//   operator int_type() const
//   {
//     return value_;
//   }
  int_adapter operator+(const int_adapter& rhs) const
  {
    if (is_nan() || rhs.is_nan()) {
      return int_adapter<int_type>(not_a_number());
    }
    if (is_infinity()) {
      return *this;
    }
    if (rhs.is_infinity()) {
      return rhs;
    }
    return value_ + rhs.value_;
  }

  int_adapter operator+(int_type rhs) const
  {
    if (is_nan()) {
      return int_adapter<int_type>(not_a_number());
    }
    if (is_infinity()) {
      return *this;
    }
    return value_ + rhs;
  }

  int_adapter operator-(const int_adapter& rhs) const
  {
    if (is_nan() || rhs.is_nan()) {
      return int_adapter<int_type>(not_a_number());
    }
    if (is_infinity()) {
      return *this;
    }
    if (rhs.is_infinity()) {
      return rhs;
    }
    return int_adapter<int_type>(value_ - rhs.value_);
  }

  int_adapter operator-(int_type rhs) const
  {
    if (is_nan()) {
      return int_adapter<int_type>(not_a_number());
    }
    if (is_infinity()) {
      return *this;
    }
    return int_adapter<int_type>(value_ - rhs);
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
