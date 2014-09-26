// (C) Copyright Jens Maurer 2001. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// Revision History:

// 15 Nov 2001   Jens Maurer
//      created.

//  See http://www.boost.org/libs/utility/iterator_adaptors.htm for documentation.

#ifndef BOOST_ITERATOR_ADAPTOR_GENERATOR_ITERATOR_HPP
#define BOOST_ITERATOR_ADAPTOR_GENERATOR_ITERATOR_HPP

#include <boost/iterator_adaptors.hpp>
#include <boost/ref.hpp>

namespace boost {

template<class Generator>
class generator_iterator_policies
{
public:
    generator_iterator_policies() { }

    template<class Base>
    void initialize(Base& base) {
      m_value = (*base)();
    }

    // The Iter template argument is necessary for compatibility with a MWCW
    // bug workaround
    template <class IteratorAdaptor>
    void increment(IteratorAdaptor& iter) {
      m_value = (*iter.base())();
    }

    template <class IteratorAdaptor>
    const typename Generator::result_type&
    dereference(const IteratorAdaptor&) const
        { return m_value; }

    template <class IteratorAdaptor1, class IteratorAdaptor2>
    bool equal(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
        { return x.base() == y.base() &&
            x.policies().m_value == y.policies().m_value; }

private:
  typename Generator::result_type m_value;
};

template<class Generator>
struct generator_iterator_generator
{
  typedef iterator_adaptor<Generator*, generator_iterator_policies<Generator>,
    typename Generator::result_type, const typename Generator::result_type&,
    const typename Generator::result_type*, std::input_iterator_tag,
    long>       type;
};

template <class Generator>
inline typename generator_iterator_generator<Generator>::type
make_generator_iterator(Generator & gen)
{
  typedef typename generator_iterator_generator<Generator>::type result_t;
  return result_t(&gen);
}

} // namespace boost


#endif // BOOST_ITERATOR_ADAPTOR_GENERATOR_ITERATOR_HPP

