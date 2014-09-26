// Copyright (C) 2002 Ronald Garcia
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies. 
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice 
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty, 
// and with no claim as to its suitability for any purpose.
//

#ifndef COPY_ARRAY_RG092101_HPP
#define COPY_ARRAY_RG092101_HPP

//
// copy_array.hpp - generic code for copying the contents of one
// Basic_MultiArray to another.  We assume that they are of the same
// shape
//
#include "boost/type.hpp"
#include <cassert>

namespace boost {
namespace detail {
namespace multi_array {

template <typename Element>
class copy_dispatch {
public:
  template <typename SourceIterator, typename DestIterator>
  static void copy_array (SourceIterator first, SourceIterator last,
                   DestIterator result) {
    while (first != last) {
      copy_array(*first++,*result++);
    }
  }
private:
  // Array2 has to be passed by VALUE here because subarray
  // pseudo-references are temporaries created by iterator::operator*()
  template <typename Array1, typename Array2>
  static void copy_array (const Array1& source, Array2 dest) {
    copy_array(source.begin(),source.end(),dest.begin());
  }

  static void copy_array (const Element& source, Element& dest) {
    dest = source;
  }

};


template <typename Array1, typename Array2>
void copy_array (Array1& source, Array2& dest) {
  assert(std::equal(source.shape(),source.shape()+source.num_dimensions(),
                    dest.shape()));
  // Dispatch to the proper function
  typedef typename Array1::element element_type;
  copy_dispatch<element_type>::
    copy_array(source.begin(),source.end(),dest.begin());
}


} // namespace multi_array
} // namespace detail
} // namespace boost

#endif // COPY_ARRAY_RG092101_HPP
