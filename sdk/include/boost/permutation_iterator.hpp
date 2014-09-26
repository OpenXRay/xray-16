// (C) Copyright Toon Knapen 2001. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//

//  See http://www.boost.org/libs/utility/permutation_iterator.htm for documentation.

#ifndef boost_permutation_iterator_hpp
#define boost_permutation_iterator_hpp

#include <boost/iterator_adaptors.hpp>

namespace boost {

  template < typename IndexIterator >
  struct permutation_iterator_policies : public default_iterator_policies
  {
    permutation_iterator_policies() {}

    permutation_iterator_policies(IndexIterator order_it) 
      : order_it_( order_it ) 
    {}

    template <class IteratorAdaptor>
    typename IteratorAdaptor::reference dereference(const IteratorAdaptor& x) const
    { return *(x.base() + *order_it_); }

    template <class IteratorAdaptor>
    void increment(IteratorAdaptor&)
    { ++order_it_; }

    template <class IteratorAdaptor>
    void decrement(IteratorAdaptor&)
    { --order_it_; }

    template <class IteratorAdaptor, class DifferenceType>
    void advance(IteratorAdaptor& x, DifferenceType n)
    { std::advance( order_it_, n ); }

    template <class IteratorAdaptor1, class IteratorAdaptor2>
    typename IteratorAdaptor1::difference_type
    distance(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
    { return std::distance( x.policies().order_it_, y.policies().order_it_ ); }

    template <class IteratorAdaptor1, class IteratorAdaptor2>
    bool equal(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
    { return x.policies().order_it_ == y.policies().order_it_; }
  
    IndexIterator order_it_;
  };

  template < typename ElementIterator, typename IndexIterator >
  struct permutation_iterator_generator
  {
    typedef boost::iterator_adaptor
    < ElementIterator,
      permutation_iterator_policies< IndexIterator > 
    > type;
  };

  template < class IndexIterator, class ElementIterator >
  inline typename permutation_iterator_generator< ElementIterator, IndexIterator >::type
  make_permutation_iterator(ElementIterator base, IndexIterator order)
  {
    typedef typename permutation_iterator_generator< ElementIterator, IndexIterator >::type result_t;
    return result_t( base, order );
  }

} // namespace boost

#endif // boost_permutation_iterator_hpp

