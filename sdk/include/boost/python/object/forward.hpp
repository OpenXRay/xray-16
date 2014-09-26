// Copyright David Abrahams 2001. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef FORWARD_DWA20011215_HPP
# define FORWARD_DWA20011215_HPP

# include <boost/mpl/if.hpp>
# include <boost/type_traits/object_traits.hpp>
# include <boost/type_traits/composite_traits.hpp>
# include <boost/type_traits/transform_traits.hpp>
# include <boost/type_traits/add_const.hpp>
# include <boost/ref.hpp>

namespace boost { namespace python { namespace objects { 

// Very much like boost::reference_wrapper<T>, except that in this
// case T can be a reference already without causing a
// reference-to-reference error.
template <class T>
struct reference_to_value
{
    typedef typename add_reference<typename add_const<T>::type>::type reference;
    
    reference_to_value(reference x) : m_value(x) {}
    operator reference() const { return m_value; }
 private:
    reference m_value;
};

// A little metaprogram which selects the type to pass through an
// intermediate forwarding function when the destination argument type
// is T.
template <class T>
struct forward
    : mpl::if_c<
        is_scalar<T>::value
        , T
        , reference_to_value<T> >
{
};

# ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
template<typename T>
class unforward
{
 public:
    typedef typename unwrap_reference<T>::type& type;
};

template<typename T>
class unforward<reference_to_value<T> >
{
 public:
    typedef T type;
};
# else // no partial specialization

namespace detail
{
  typedef char (&yes_reference_to_value_t)[1];
  typedef char (&no_reference_to_value_t)[2];
      
  no_reference_to_value_t is_reference_to_value_test(...);

  template<typename T>
  yes_reference_to_value_t is_reference_to_value_test(boost::type< reference_to_value<T> >);

  template<bool wrapped>
  struct unforwarder
  {
      template <class T>
      struct apply
      {
          typedef typename unwrap_reference<T>::type& type;
      };
  };

  template<>
  struct unforwarder<true>
  {
      template <class T>
      struct apply
      {
          typedef typename T::reference type;
      };
  };

  template<typename T>
  class is_reference_to_value
  {
   public:
      BOOST_STATIC_CONSTANT(
          bool, value = (
              sizeof(is_reference_to_value_test(boost::type<T>()))
              == sizeof(yes_reference_to_value_t)));
  };
}

template <typename T>
class unforward
    : public detail::unforwarder<
        detail::is_reference_to_value<T>::value
      >::template apply<T>
{};

# endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

}}} // namespace boost::python::objects

#endif // FORWARD_DWA20011215_HPP
