// (C) Copyright Ronald Garcia 2002. Permission to copy, use, modify, sell and
// distribute this software is granted provided this copyright notice appears
// in all copies. This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.

// See http://www.boost.org/libs/utility/shared_container_iterator.html for documentation.

#ifndef SHARED_CONTAINER_ITERATOR_RG08102002_HPP
#define SHARED_CONTAINER_ITERATOR_RG08102002_HPP

#include "boost/iterator_adaptors.hpp"
#include "boost/shared_ptr.hpp"
#include <utility>

namespace boost {

template <typename Container>
struct shared_container_iterator_policies :
  public boost::default_iterator_policies {
  typedef boost::shared_ptr<Container> container_ref_t;
  container_ref_t container_ref;
  shared_container_iterator_policies(container_ref_t const& c) :
    container_ref(c) { }
  shared_container_iterator_policies() { }
};


template <typename Container>
class shared_container_iterator_generator {
  typedef typename Container::iterator iterator;
  typedef shared_container_iterator_policies<Container> policy;
public:
  typedef boost::iterator_adaptor<iterator,policy> type;
};

template <typename Container>
typename shared_container_iterator_generator<Container>::type
make_shared_container_iterator(typename Container::iterator iter,
			       boost::shared_ptr<Container> const& container) {
  typedef typename shared_container_iterator_generator<Container>::type
    iterator;
  typedef shared_container_iterator_policies<Container> policy;
  return iterator(iter,policy(container));
}

template <typename Container>
std::pair<
  typename shared_container_iterator_generator<Container>::type,
  typename shared_container_iterator_generator<Container>::type>
make_shared_container_range(boost::shared_ptr<Container> const& container) {
  return
    std::make_pair(
      make_shared_container_iterator(container->begin(),container),
      make_shared_container_iterator(container->end(),container));
}


} // namespace boost
#endif  // SHARED_CONTAINER_ITERATOR_RG08102002_HPP
