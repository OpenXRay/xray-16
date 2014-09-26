/* boost random/uniform_real.hpp header file
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
 * $Id: uniform_real.hpp,v 1.10 2002/12/22 22:03:11 jmaurer Exp $
 *
 * Revision history
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_REAL_HPP
#define BOOST_RANDOM_UNIFORM_REAL_HPP

#include <cassert>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/uniform_01.hpp>

namespace boost {

// uniform distribution on a real range
template<class UniformRandomNumberGenerator, class RealType = double,
        class Adaptor = uniform_01<UniformRandomNumberGenerator, RealType> >
class uniform_real
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef RealType result_type;
  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

  explicit uniform_real(base_type & rng, RealType min = RealType(0),
                        RealType max = RealType(1)) 
    : _rng(rng), _min(min), _max(max)
  {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(!std::numeric_limits<RealType>::is_integer);
#endif
    assert(min < max);
  }

  // compiler-generated copy ctor and assignment operator are fine

  result_type min() const { return _min; }
  result_type max() const { return _max; }
  adaptor_type& adaptor() { return _rng; }
  base_type& base() const { return _rng.base(); }
  void reset() { _rng.reset(); }

  result_type operator()() { return _rng() * (_max - _min) + _min; }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const uniform_real& x, const uniform_real& y)
  { return x._min == y._min && x._max == y._max && x._rng == y._rng; }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_real& ud)
  {
    os << ud._min << " " << ud._max;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_real& ud)
  {
    is >> std::ws >> ud._min >> std::ws >> ud._max;
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const uniform_real& rhs) const
  { return _min == rhs._min && _max == rhs._max && _rng == rhs._rng;  }
#endif
private:
  adaptor_type _rng;
  RealType _min, _max;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class UniformRandomNumberGenerator, class RealType, class Adaptor>
const bool uniform_real<UniformRandomNumberGenerator, RealType, Adaptor>::has_fixed_range;
#endif

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_REAL_HPP
