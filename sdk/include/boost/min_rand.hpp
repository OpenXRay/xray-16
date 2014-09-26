//  Boost min_rand.hpp header file  ------------------------------------------//

//  (C) Copyright Beman Dawes 1998. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

//  Version 1.1, 25 May 99  Add operator()() to meet Generator requirements
//  Version 1.0,  9 Nov 98  Initial version

#ifndef BOOST_MIN_RAND_HPP
#define BOOST_MIN_RAND_HPP

#include <cassert>

namespace boost {

//  min_rand  ----------------------------------------------------------------//

//  see min_rand.html for documentation

class min_rand {

  //  Don't even think about changing the values of the constants below.
  //  See the article cited in the documentation for rationale.
  enum constants {
    modulus = 2147483647L,
    multiplier = 48271L,          // 16807L for original "minimal standard"
    validation = 399268537L,      // 1043618065L for original "minimal standard"
    q = modulus / multiplier, 
    r = modulus % multiplier
    };

  long value;                     // invariant: 0 < value <= modulus

 public:

  //  compiler generated copy constructor and operator= are valid and useful

  explicit min_rand( long seed_value=1 ) : value( seed_value )
                              { assert( value > 0 && value <= modulus ); }

  operator long() const       { return value; }
  double fvalue() const       { return double(value) / modulus; }

  min_rand& operator=( long new_value ) {
                                value = new_value;
                                assert( value > 0 && value <= modulus );
                                return *this;
                              }

  long operator++()           { value = multiplier*(value%q) - r*(value/q);
                                if ( value <= 0 ) value += modulus;
                                assert( value > 0 && value <= modulus );
                                return value;
                              }
  long operator++(int)        { long temp = value; operator++(); return temp; }

  long ten_thousandth() const { return validation; }

  //  satisfy std::RandomNumberGenerator and std::Generator requirements:
  typedef long  argument_type;
  typedef long  result_type;
  long operator()( long n )   { return operator++() % n; }
  long operator()()           { return operator++(); }

  }; // min_rand

} // namespace boost

#endif  // BOOST_MIN_RAND_HPP
