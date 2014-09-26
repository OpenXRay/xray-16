// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef REGISTERED_DWA2002710_HPP
# define REGISTERED_DWA2002710_HPP
# include <boost/python/type_id.hpp>
# include <boost/python/converter/registry.hpp>
# include <boost/python/converter/registrations.hpp>
# include <boost/type_traits/transform_traits.hpp>
# include <boost/type_traits/cv_traits.hpp>

namespace boost { namespace python { namespace converter { 

struct registration;

namespace detail
{
  template <class T>
  struct registered_base
  {
      static registration const& converters;
  };
}

template <class T>
struct registered
    : detail::registered_base<
        typename add_reference<
           typename add_cv<T>::type
        >::type
    >
{
};

# ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
// collapses a few more types to the same static instance
template <class T>
struct registered<T&> : registered<T> {};
# endif

//
// implementations
//
namespace detail
{
  template <class T>
  registration const& registered_base<T>::converters
     = registry::lookup(type_id<T>());
}
}}} // namespace boost::python::converter

#endif // REGISTERED_DWA2002710_HPP
