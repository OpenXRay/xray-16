/* boost random/uniform_01.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Permission to use, copy, modify, sell, and distribute this software
 * is hereby granted without fee provided that the above copyright notice
 * appears in all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation,
 *
 * Jens Maurer makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: uniform_01.hpp,v 1.13 2002/12/22 22:03:11 jmaurer Exp $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_01_HPP
#define BOOST_RANDOM_UNIFORM_01_HPP

#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>

namespace boost {

// Because it is so commonly used: uniform distribution on the real [0..1)
// range.  This allows for specializations to avoid a costly int -> float
// conversion plus float multiplication
template<class UniformRandomNumberGenerator, class RealType = double>
class uniform_01
{
public:
  typedef uniform_01<UniformRandomNumberGenerator, RealType> adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef RealType result_type;

  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
  BOOST_STATIC_ASSERT(!std::numeric_limits<RealType>::is_integer);
#endif

  explicit uniform_01(base_type & rng)
    : _rng(&rng),
      _factor(result_type(1) /
              (result_type(_rng->max()-_rng->min()) +
               result_type(std::numeric_limits<base_result>::is_integer ? 1 : 0)))
  {
  }
  // compiler-generated copy ctor and copy assignment are fine

  result_type min() const { return result_type(0); }
  result_type max() const { return result_type(1); }
  adaptor_type& adaptor() { return *this; }
  base_type& base() const { return *_rng; }
  void reset() { }

  result_type operator()() {
    return result_type((*_rng)() - _rng->min()) * _factor;
  }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const uniform_01& x, const uniform_01& y)
  { return *x._rng == *y._rng; }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_01&)
  {
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_01&)
  {
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const uniform_01& rhs) const
  { return *_rng == *rhs._rng;  }
#endif
private:
  typedef typename base_type::result_type base_result;
  base_type * _rng;
  result_type _factor;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class UniformRandomNumberGenerator, class RealType>
const bool uniform_01<UniformRandomNumberGenerator, RealType>::has_fixed_range;
#endif

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_01_HPP
