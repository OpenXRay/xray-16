// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef KEYWORDS_DWA2002323_HPP
# define KEYWORDS_DWA2002323_HPP

# include <boost/python/args_fwd.hpp>
# include <boost/config.hpp>
# include <boost/python/detail/preprocessor.hpp>
# include <boost/python/detail/type_list.hpp>

# include <boost/type_traits/is_reference.hpp>
# include <boost/type_traits/remove_reference.hpp>
# include <boost/type_traits/remove_cv.hpp>

# include <boost/preprocessor/enum_params.hpp>
# include <boost/preprocessor/repeat.hpp>
# include <boost/preprocessor/facilities/intercept.hpp>
# include <boost/preprocessor/iteration/local.hpp>

# include <boost/python/detail/mpl_lambda.hpp>
# include <boost/mpl/bool.hpp>

# include <boost/type.hpp>
# include <cstddef>


namespace boost { namespace python {

namespace detail
{
  template <std::size_t nkeywords>
  struct keywords
  {
      BOOST_STATIC_CONSTANT(std::size_t, size = nkeywords);
      
      keyword_range range() const
      {
          return keyword_range(elements, elements + nkeywords);
      }
      
      keyword elements[nkeywords];
  };

# ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
  template<typename T>
  struct is_keywords
  {
      BOOST_STATIC_CONSTANT(bool, value = false); 
  };

  template<std::size_t nkeywords>
  struct is_keywords<keywords<nkeywords> >
  {
      BOOST_STATIC_CONSTANT(bool, value = true);
  };
  template <class T>
  struct is_reference_to_keywords
  {
      BOOST_STATIC_CONSTANT(bool, is_ref = is_reference<T>::value);
      typedef typename remove_reference<T>::type deref;
      typedef typename remove_cv<deref>::type key_t;
      BOOST_STATIC_CONSTANT(bool, is_key = is_keywords<key_t>::value);
      BOOST_STATIC_CONSTANT(bool, value = (is_ref & is_key));
      
      typedef mpl::bool_<value> type;
      BOOST_PYTHON_MPL_LAMBDA_SUPPORT(1,is_reference_to_keywords,(T))
  };
# else 
  typedef char (&yes_keywords_t)[1];
  typedef char (&no_keywords_t)[2];
      
  no_keywords_t is_keywords_test(...);

  template<std::size_t nkeywords>
  yes_keywords_t is_keywords_test(void (*)(keywords<nkeywords>&));

  template<std::size_t nkeywords>
  yes_keywords_t is_keywords_test(void (*)(keywords<nkeywords> const&));

  template<typename T>
  class is_reference_to_keywords
  {
   public:
      BOOST_STATIC_CONSTANT(
          bool, value = (
              sizeof(detail::is_keywords_test( (void (*)(T))0 ))
              == sizeof(detail::yes_keywords_t)));

      typedef mpl::bool_<value> type;
      BOOST_PYTHON_MPL_LAMBDA_SUPPORT(1,is_reference_to_keywords,(T))
  };
# endif 
}

#  define BOOST_PYTHON_ASSIGN_NAME(z, n, _) result.elements[n].name = name##n;
#  define BOOST_PP_LOCAL_MACRO(n)                                               \
inline detail::keywords<n> args(BOOST_PP_ENUM_PARAMS_Z(1, n, char const* name)) \
{                                                                               \
    detail::keywords<n> result;                                                 \
    BOOST_PP_REPEAT_1(n, BOOST_PYTHON_ASSIGN_NAME, _)                           \
    return result;                                                              \
}
#  define BOOST_PP_LOCAL_LIMITS (1, BOOST_PYTHON_MAX_ARITY)
#  include BOOST_PP_LOCAL_ITERATE()

}} // namespace boost::python


# endif // KEYWORDS_DWA2002323_HPP
