// Copyright David Abrahams 2001. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef MAKE_FUNCTION_DWA20011221_HPP
# define MAKE_FUNCTION_DWA20011221_HPP

# include <boost/python/default_call_policies.hpp>
# include <boost/python/args_fwd.hpp>
# include <boost/python/detail/caller.hpp>

# include <boost/python/object/function_object.hpp>

# include <boost/mpl/size.hpp>
# include <boost/mpl/int.hpp>

namespace boost { namespace python {

namespace detail
{
  // make_function_aux --
  //
  // These helper functions for make_function (below) do the raw work
  // of constructing a Python object from some invokable entity. See
  // <boost/python/detail/caller.hpp> for more information about how
  // the ConverterGenerators and Sig arguments are used.
  template <class F, class CallPolicies, class ConverterGenerators, class Sig>
  object make_function_aux(
      F f                               // An object that can be invoked by detail::invoke()
      , CallPolicies const& p           // CallPolicies to use in the invocation
      , ConverterGenerators const&      // An MPL iterator over a sequence of arg_from_python generators
      , Sig const&                      // An MPL sequence of argument types expected by F
      )
  {
      return objects::function_object(
          detail::caller<F,ConverterGenerators,CallPolicies,Sig>(f, p)
          , mpl::size<Sig>::value - 1);
  }

  // As above, except that it accepts argument keywords. NumKeywords
  // is used only for a compile-time assertion to make sure the user
  // doesn't pass more keywords than the function can accept. To
  // disable all checking, pass mpl::int_<0> for NumKeywords.
  template <class F, class CallPolicies, class ConverterGenerators, class Sig, class NumKeywords>
  object make_function_aux(
      F f
      , CallPolicies const& p
      , ConverterGenerators const&
      , Sig const&
      , detail::keyword_range const& kw // a [begin,end) pair of iterators over keyword names
      , NumKeywords                     // An MPL integral type wrapper: the size of kw
      )
  {
      enum { arity = mpl::size<Sig>::value - 1 };
      
      typedef typename detail::error::more_keywords_than_function_arguments<
          NumKeywords::value, arity
          >::too_many_keywords assertion;
    
      return objects::function_object(
          detail::caller<F,ConverterGenerators,CallPolicies,Sig>(f, p)
          , arity
          , kw);
  }
}

// make_function --
//
// These overloaded functions wrap a function or member function
// pointer as a Python object, using optional CallPolicies and
// Keywords.
template <class F>
object make_function(F f)
{
    return detail::make_function_aux(
        f,default_call_policies(),detail::args_from_python(), detail::get_signature(f));
}

template <class F, class CallPolicies>
object make_function(F f, CallPolicies const& policies)
{
    return detail::make_function_aux(
        f,policies,detail::args_from_python(), detail::get_signature(f));
}

template <class F, class CallPolicies, class Keywords>
object make_function(F f, CallPolicies const& policies, Keywords const& keywords)
{
    return detail::make_function_aux(
        f
        , policies
        , detail::args_from_python()
        , detail::get_signature(f)
        , keywords.range()
        , mpl::int_<Keywords::size>()
        );
}

}} 


#endif // MAKE_FUNCTION_DWA20011221_HPP
