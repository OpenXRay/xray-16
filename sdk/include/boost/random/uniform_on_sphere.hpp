/* boost random/uniform_on_sphere.hpp header file
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
 * $Id: uniform_on_sphere.hpp,v 1.9 2002/12/22 22:03:11 jmaurer Exp $
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_ON_SPHERE_HPP
#define BOOST_RANDOM_UNIFORM_ON_SPHERE_HPP

#include <vector>
#include <algorithm>     // std::transform
#include <functional>    // std::bind2nd, std::divides
#include <boost/random/normal_distribution.hpp>

namespace boost {

template<class UniformRandomNumberGenerator, class RealType = double,
         class Cont = std::vector<RealType>,
         class Adaptor = uniform_01<UniformRandomNumberGenerator, RealType> >
class uniform_on_sphere
{
public:
  typedef Adaptor adaptor_type;
  typedef UniformRandomNumberGenerator base_type;
  typedef Cont result_type;

  explicit uniform_on_sphere(base_type & rng, int dim = 2)
    : _rng(rng), _container(dim), _dim(dim) { }

  // compiler-generated copy ctor and assignment operator are fine

  adaptor_type& adaptor() { return _rng.adaptor(); }
  base_type& base() const { return _rng.base(); }
  void reset() { _rng.reset(); }

  const result_type & operator()()
  {
    RealType sqsum = 0;
    for(typename Cont::iterator it = _container.begin();
        it != _container.end();
        ++it) {
      RealType val = _rng();
      *it = val;
      sqsum += val * val;
    }
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif
    // for all i: result[i] /= sqrt(sqsum)
    std::transform(_container.begin(), _container.end(), _container.begin(),
                   std::bind2nd(std::divides<RealType>(), sqrt(sqsum)));
    return _container;
  }

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
  friend bool operator==(const uniform_on_sphere& x, 
                         const uniform_on_sphere& y)
  { return x._dim == y._dim && x._rng == y._rng; }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  template<class CharT, class Traits>
  friend std::basic_ostream<CharT,Traits>&
  operator<<(std::basic_ostream<CharT,Traits>& os, const uniform_on_sphere& sd)
  {
    os << sd._dim;
    return os;
  }

  template<class CharT, class Traits>
  friend std::basic_istream<CharT,Traits>&
  operator>>(std::basic_istream<CharT,Traits>& is, uniform_on_sphere& sd)
  {
    is >> std::ws >> sd._dim;
    sd._container.resize(sd._dim);
    return is;
  }
#endif

#else
  // Use a member function
  bool operator==(const uniform_on_sphere& rhs) const
  { return _dim == rhs._dim && _rng == rhs._rng; }
#endif
private:
  normal_distribution<base_type, RealType, Adaptor> _rng;
  result_type _container;
  int _dim;
};

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_ON_SPHERE_HPP
