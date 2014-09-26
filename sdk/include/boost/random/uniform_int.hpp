/* boost random/uniform_int.hpp header file
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
 * $Id: uniform_int.hpp,v 1.21.2.1 2003/03/17 01:39:15 beman_dawes Exp $
 *
 * Revision history
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_INT_HPP
#define BOOST_RANDOM_UNIFORM_INT_HPP

#include <cassert>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/random/detail/signed_unsigned_compare.hpp>
#ifdef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
#include <boost/type_traits/is_float.hpp>
#endif

namespace boost {

// uniform integer distribution on [min, max]

namespace detail {

template<class UniformRandomNumberGenerator, class IntType>
class uniform_int_integer
{
public:
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  uniform_int_integer(base_type & rng, IntType min, IntType max) 
    : _rng(&rng), _bmin(_rng->min()), _brange(_rng->max() - _bmin)
  {
    assert(min < max);
    set(min, max);
  }

  void set(result_type min, result_type max)
  {
    _min = min;
    _max = max;
    _range = _max - _min;
    
    if(random::equal_signed_unsigned(_brange, _range))
      _range_comparison = 0;
    else if(random::lessthan_signed_unsigned(_brange, _range))
      _range_comparison = -1;
    else
      _range_comparison = 1;
  }

  result_type min() const { return _min; }
  result_type max() const { return _max; }
  base_type& base() const { return *_rng; }

  result_type operator()();

private:
  typedef typename base_type::result_type base_result;
  base_type * _rng;
  result_type _min, _max, _range;
  base_result _bmin, _brange;
  int _range_comparison;
};

template<class UniformRandomNumberGenerator, class IntType>
inline IntType uniform_int_integer<UniformRandomNumberGenerator, IntType>::operator()()
{
  if(_range_comparison == 0) {
    // this will probably never happen in real life
    // basically nothing to do; just take care we don't overflow / underflow
    return static_cast<result_type>((*_rng)() - _bmin) + _min;
  } else if(_range_comparison < 0) {
    // use rejection method to handle things like 0..3 --> 0..4
    for(;;) {
      // concatenate several invocations of the base RNG
      // take extra care to avoid overflows
      result_type limit;
      if(_range == std::numeric_limits<result_type>::max()) {
        limit = _range/(result_type(_brange)+1);
        if(_range % result_type(_brange)+1 == result_type(_brange))
          ++limit;
      } else {
        limit = (_range+1)/(result_type(_brange)+1);
      }
      // we consider "result" as expressed to base (_brange+1)
      // for every power of (_brange+1), we determine a random factor
      result_type result = result_type(0);
      result_type mult = result_type(1);
      while(mult <= limit) {
        result += ((*_rng)() - _bmin) * mult;
        mult *= result_type(_brange)+result_type(1);
      }
      if(mult == limit)
        // _range+1 is an integer power of _brange+1: no rejections required
        return result;
      // _range/mult < _brange+1  -> no endless loop
      result += uniform_int_integer<base_type,result_type>(*_rng, 0, _range/mult)() * mult;
      if(result <= _range)
        return result + _min;
    }
  } else {                   // brange > range
    if(_brange / _range > 4 /* quantization_cutoff */ ) {
      // the new range is vastly smaller than the source range,
      // so quantization effects are not relevant
      return boost::uniform_smallint<base_type,result_type>(*_rng, _min, _max)();
    } else {
      // use rejection method to handle things like 0..5 -> 0..4
      for(;;) {
        base_result result = (*_rng)() - _bmin;
        // result and range are non-negative, and result is possibly larger
        // than range, so the cast is safe
        if(result <= static_cast<base_result>(_range))
          return result + _min;
      }
    }
  }
}


template<class UniformRandomNumberGenerator, class IntType>
class uniform_int_float
{
public:
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  uniform_int_float(base_type & rng, IntType min, IntType max)
    : _rng(rng)
  {
    set(min,max);
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
  result_type _min, _max;
  base_result _range;
};


// simulate partial specialization
template<bool is_integer>
struct uniform_int;

template<>
struct uniform_int<true>
{
  template<class UniformRandomNumberGenerator, class IntType>
  struct impl
  {
    typedef uniform_int_integer<UniformRandomNumberGenerator, IntType> type
;
  };
};

template<>
struct uniform_int<false>
{
  template<class UniformRandomNumberGenerator, class IntType>
  struct impl
  {
    typedef uniform_int_float<UniformRandomNumberGenerator, IntType> type;
  };
};

} // namespace detail


template<class UniformRandomNumberGenerator, class IntType = int>
class uniform_int
{
private:
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
  typedef typename detail::uniform_int<std::numeric_limits<typename UniformRandomNumberGenerator::result_type>::is_integer>::BOOST_NESTED_TEMPLATE impl<UniformRandomNumberGenerator, IntType>::type impl_type;
#elif BOOST_WORKAROUND( __BORLANDC__, BOOST_TESTED_AT(0x0570) )
  typedef typename detail::uniform_int< boost::is_float<typename UniformRandomNumberGenerator::result_type>::value == false >::BOOST_NESTED_TEMPLATE impl<UniformRandomNumberGenerator, IntType>::type impl_type;
#else
  BOOST_STATIC_CONSTANT(bool, base_float = (boost::is_float<typename UniformRandomNumberGenerator::result_type>::value == false));
  typedef typename detail::uniform_int<base_float>::BOOST_NESTED_TEMPLATE impl<UniformRandomNumberGenerator, IntType>::type impl_type;
#endif

public:
  typedef uniform_int<UniformRandomNumberGenerator, IntType> adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef IntType result_type;

  BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

  explicit uniform_int(base_type & rng, IntType min = 0, IntType max = 9)
    : impl(rng, min, max)
  {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    // MSVC fails BOOST_STATIC_ASSERT with std::numeric_limits at class scope
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
#endif
  }

  result_type min() const { return impl.min(); }
  result_type max() const { return impl.max(); }
  adaptor_type& adaptor() { return *this; }
  base_type& base() const { return impl.base(); }
  void reset() { }

  result_type operator()() { return impl(); }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const uniform_int& x, const uniform_int& y)
  { return x.min() == y.min() && x.max() == y.max() && x.base() == y.base(); }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_int& ud)
  {
    os << ud.min() << " " << ud.max();
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_int& ud)
  {
# if BOOST_WORKAROUND(_MSC_FULL_VER, BOOST_TESTED_AT(13102292)) && BOOST_MSVC > 1300
      return detail::extract_uniform_int(is, ud, ud.impl);
# else
    IntType min, max;
    is >> std::ws >> min >> std::ws >> max;
    ud.impl.set(min, max);
    return is;
# endif 
  }
#endif

#else
  // Use a member function
  bool operator==(const uniform_int& rhs) const
  { return min() == rhs.min() && max() == rhs.max() && base() == rhs.base();  }
#endif
private:
  impl_type impl;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class UniformRandomNumberGenerator, class IntType>
const bool uniform_int<UniformRandomNumberGenerator, IntType>::has_fixed_range;
#endif

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_INT_HPP
