// (C) Copyright Jeremy Siek, 2001. Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

//  See http://www.boost.org/libs/property_map for documentation.

#ifndef BOOST_PROPERTY_MAP_ITERATOR_HPP
#define BOOST_PROPERTY_MAP_ITERATOR_HPP

#include <boost/property_map.hpp>
#include <boost/iterator_adaptors.hpp>

namespace boost {

  //======================================================================
  // property iterator, generalized from ideas by François Faure

  namespace detail {

    template <class LvaluePropertyMap>
    struct lvalue_pmap_iter_policies : public default_iterator_policies
    {
      lvalue_pmap_iter_policies() { }
      lvalue_pmap_iter_policies(LvaluePropertyMap m) : m_map(m) {}

      template <class Iter>
      typename Iter::reference
      dereference(const Iter& i) const 
      {
        return m_map[*i.base()];
      }
    private:
      LvaluePropertyMap m_map;
    };

    template <class ReadablePropertyMap>
    struct readable_pmap_iter_policies : public default_iterator_policies
    {
      readable_pmap_iter_policies() { }
      readable_pmap_iter_policies(ReadablePropertyMap m) : m_map(m) {}

      template <class Iter>
      typename Iter::reference
      dereference(const Iter& i) const 
      {
        return get(m_map, *i.base());
      }
    private:
      ReadablePropertyMap m_map;
    };

    template <class PMapCategory>
    struct choose_pmap_iter {
      template <class PMap, class Iter>
      struct bind_ {     
        typedef typename property_traits<PMap>::value_type value;
        typedef iterator_adaptor<Iter,
          readable_pmap_iter_policies<PMap>, value, value,
          value*, std::input_iterator_tag> type;
      };
    };

    template <>
    struct choose_pmap_iter<lvalue_property_map_tag> {
      template <class PMap, class Iter>
      struct bind_ {
        typedef typename property_traits<PMap>::value_type value;
        typedef typename property_traits<PMap>::reference ref;
        typedef iterator_adaptor<Iter,
          lvalue_pmap_iter_policies<PMap>,
          value, ref> type;
      };
    };
    
  } // namespace detail

  template <class PropertyMap, class Iterator>
  class property_map_iterator_generator {
  public:
    typedef typename property_traits<PropertyMap>::category Cat; 
    typedef typename detail::choose_pmap_iter<Cat>::
      template bind_<PropertyMap, Iterator>::type type;
  };

  template <class PropertyMap, class Iterator>
  typename property_map_iterator_generator<PropertyMap, Iterator>::type
  make_property_map_iterator(PropertyMap pmap, Iterator iter)
  {
    typedef typename property_map_iterator_generator<PropertyMap, 
      Iterator>::type Iter;
    return Iter(iter, pmap);
  }

} // namespace boost

#endif // BOOST_PROPERTY_MAP_ITERATOR_HPP

