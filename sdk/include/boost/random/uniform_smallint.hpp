/* boost random/uniform_smallint.hpp header file
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
 * $Id: uniform_smallint.hpp,v 1.20.2.1 2003/03/17 01:39:15 beman_dawes Exp $
 *
 * Revision history
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_SMALLINT_HPP
#define BOOST_RANDOM_UNIFORM_SMALLINT_HPP

#include <cassert>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/detail/workaround.hpp>
#ifdef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
#include <boost/type_traits/is_float.hpp>
#endif


namespace boost {

// uniform integer distribution on a small range [min, max]

namespace detail {

template <class InputStream, class UniformInt, class Impl>
InputStream& extract_uniform_int(InputStream& is, UniformInt& ud, Impl& impl)
{
    typename UniformInt::result_type min, max;
    is >> std::ws >> min >> std::ws >> max;
    impl.set(min, max);
    return is;
}

template<class UniformRandomNumberGenerator, class IntType>
struct uniform_smallint_integer
{
public:
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  uniform_smallint_integer(base_type & rng, IntType min, IntType max)
    : _rng(&rng)
  { set(min, max); }

  void set(result_type min, result_type max);
  
  result_type min() const { return _min; }
  result_type max() const { return _max; }
  base_type& base() const { return *_rng; }
  
  result_type operator()()
  {
    // we must not use the low bits here, because LCGs get very bad then
    return (((*_rng)() - _rng->min()) / _factor) % _range + _min;
  }

private:
  typedef typename base_type::result_type base_result;
  base_type * _rng;
  IntType _min, _max;
  base_result _range;
  int _factor;
};

template<class UniformRandomNumberGenerator, class IntType>
void uniform_smallint_integer<UniformRandomNumberGenerator, IntType>::
set(result_type min, result_type max) 
{
  _min = min;
  _max = max;
  assert(min < max);

  _range = static_cast<base_result>(_max-_min)+1;
  _factor = 1;
  
  // LCGs get bad when only taking the low bits.
  // (probably put this logic into a partial template specialization)
  // Check how many low bits we can ignore before we get too much
  // quantization error.
  base_result r_base = _rng->max() - _rng->min();
  if(r_base == std::numeric_limits<base_result>::max()) {
    _factor = 2;
    r_base /= 2;
  }
  r_base += 1;
  if(r_base % _range == 0) {
    // No quantization effects, good
    _factor = r_base / _range;
  } else {
    // carefully avoid overflow; pessimizing heree
    for( ; r_base/_range/32 >= _range; _factor *= 2)
      r_base /= 2;
  }
}

template<class UniformRandomNumberGenerator, class IntType>
class uniform_smallint_float
{
public:
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  uniform_smallint_float(base_type & rng, IntType min, IntType max)
    : _rng(rng)
  {
    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
    BOOST_STATIC_ASSERT(!std::numeric_limits<typename base_type::result_type>::is_integer);
#endif

    assert(min < max);
  }

  void set(result_type min, result_type max)
  {
    _min = min;
    _max = max;
    _range = static_cast<base_result>(_max-_min)+1;
  }

  result_type min() const { return _min; }
  result_type max() const { return _max; }
  base_type& base() const { return _rng.base(); }

  result_type operator()()
  {
    return static_cast<IntType>(_rng() * _range) + _min;
  }

private:
  typedef typename base_type::result_type base_result;
  uniform_01<base_type> _rng;
  IntType _min, _max;
  base_result _range;
};


// simulate partial specialization
template<bool is_integer>
struct uniform_smallint;

template<>
struct uniform_smallint<true>
{
  template<class UniformRandomNumberGenerator, class IntType>
  struct impl
  {
    typedef uniform_smallint_integer<UniformRandomNumberGenerator, IntType> type;
  };
};

template<>
struct uniform_smallint<false>
{
  template<class UniformRandomNumberGenerator, class IntType>
  struct impl
  {
    typedef uniform_smallint_float<UniformRandomNumberGenerator, IntType> type;
  };
};

} // namespace detail




template<class UniformRandomNumberGenerator, class IntType = int>
class uniform_smallint
{
private:
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
  typedef typename detail::uniform_smallint<std::numeric_limits<typename UniformRandomNumberGenerator::result_type>::is_integer>::BOOST_NESTED_TEMPLATE impl<UniformRandomNumberGenerator, IntType>::type impl_type;
#elif BOOST_WORKAROUND( __BORLANDC__, BOOST_TESTED_AT(0x0570) )
  typedef typename detail::uniform_smallint< boost::is_float<typename UniformRandomNumberGenerator::result_type>::value == false >::BOOST_NESTED_TEMPLATE impl<UniformRandomNumberGenerator, IntType>::type impl_type;
#else
  BOOST_STATIC_CONSTANT(bool, base_float = (boost::is_float<typename UniformRandomNumberGenerator::result_type>::value == false));
  typedef typename detail::uniform_smallint<base_float>::BOOST_NESTED_TEMPLATE impl<UniformRandomNumberGenerator, IntType>::type impl_type;
#endif

public:
  typedef uniform_smallint<UniformRandomNumberGenerator, IntType> adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;
  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

  explicit uniform_smallint(base_type & rng, IntType min = 0, IntType max = 9)
    : _impl(rng, min, max)
  {
    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
#endif
  }

  result_type min() const { return _impl.min(); }
  result_type max() const { return _impl.max(); }
  adaptor_type& adaptor() { return *this; }
  base_type& base() const { return _impl.base(); }
  void reset() { }

  result_type operator()() { return _impl(); }

  // deprecated
#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const uniform_smallint& x, const uniform_smallint& y)
  { return x.min() == y.min() && x.max() == y.max() && x.base() == y.base(); }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_smallint& ud)
  {
    os << ud.min() << " " << ud.max();
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_smallint& ud)
  {
# if BOOST_WORKAROUND(_MSC_FULL_VER, BOOST_TESTED_AT(13102292)) && BOOST_MSVC > 1300
      return detail::extract_uniform_int(is, ud, ud._impl);
# else
    IntType min, max;
    is >> std::ws >> min >> std::ws >> max;
    ud._impl.set(min, max);
    return is;
# endif 
  }
#endif

#else
  // Use a member function
  bool operator==(const uniform_smallint& rhs) const
  { return min() == rhs.min() && max() == rhs.max() && base() == rhs.base();  }
#endif

private:
  impl_type _impl;  
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class UniformRandomNumberGenerator, class IntType>
const bool uniform_smallint<UniformRandomNumberGenerator, IntType>::has_fixed_range;
#endif

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_SMALLINT_HPP
