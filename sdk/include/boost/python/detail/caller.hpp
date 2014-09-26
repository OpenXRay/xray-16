#if !defined(BOOST_PP_IS_ITERATING)

// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

# ifndef CALLER_DWA20021121_HPP
#  define CALLER_DWA20021121_HPP

#  include <boost/compressed_pair.hpp>

#  include <boost/mpl/apply.hpp>
#  include <boost/mpl/if.hpp>
#  include <boost/mpl/size.hpp>
#  include <boost/type_traits/is_same.hpp>

#  include <boost/python/detail/preprocessor.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/iteration/local.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#  include <boost/preprocessor/repetition/repeat.hpp>
#  include <boost/preprocessor/cat.hpp>
#  include <boost/preprocessor/dec.hpp>
#  include <boost/preprocessor/if.hpp>

#  include <boost/python/detail/invoke.hpp>

namespace boost { namespace python { namespace detail { 

// This "result converter" is really just used as
// a dispatch tag to invoke(...), selecting the appropriate
// implementation
typedef int void_result_to_python;

// A metafunction taking an iterator FunctionIter to a metafunction
// class and an iterator ArgIter to an argument, which applies the
// result of dereferencing FunctionIter to the result of dereferencing
// ArgIter
template <class FunctionIter, class ArgIter>
struct apply_iter1
    : mpl::apply1<typename FunctionIter::type, typename ArgIter::type> {};

// Given a model of CallPolicies and a C++ result type, this
// metafunction selects the appropriate converter to use for
// converting the result to python.
template <class Policies, class Result>
struct select_result_converter
    : mpl::if_<
        is_same<Result,void>
        , void_result_to_python
        , typename mpl::apply1<typename Policies::result_converter,Result>::type*
    >
{
};


template <unsigned> struct caller_arity;

#  define BOOST_PYTHON_NEXT(init,name,n)                                                        \
     typedef BOOST_PP_IF(n,typename BOOST_PP_CAT(name,BOOST_PP_DEC(n)) ::next, init) name##n;

#  define BOOST_PYTHON_ARG_CONVERTER(n)                                         \
     BOOST_PYTHON_NEXT(typename first::next, arg_iter,n)               \
     BOOST_PYTHON_NEXT(ConverterGenerators, conv_iter,n)               \
     typedef typename apply_iter1<conv_iter##n,arg_iter##n>::type c_t##n;       \
     c_t##n c##n(PyTuple_GET_ITEM(args_, n));                                   \
     if (!c##n.convertible())                                                   \
          return 0;

#  define BOOST_PP_ITERATION_PARAMS_1                                            \
        (3, (0, BOOST_PYTHON_MAX_ARITY + 1, <boost/python/detail/caller.hpp>))
#  include BOOST_PP_ITERATE()

#  undef BOOST_PYTHON_ARG_CONVERTER
#  undef BOOST_PYTHON_NEXT

// A metafunction returning the base class used for caller<class F,
// class ConverterGenerators, class CallPolicies, class Sig>.
template <class F, class ConverterGenerators, class CallPolicies, class Sig>
struct caller_base_select
{
    enum { arity = mpl::size<Sig>::value - 1 };
    typedef typename caller_arity<arity>::template impl<F,ConverterGenerators,CallPolicies,Sig> type;
};

// A function object type which wraps C++ objects as Python callable
// objects.
//
// Template Arguments:
//
//   F -
//      the C++ `function object' that will be called. Might
//      actually be any data for which an appropriate invoke_tag() can
//      be generated. invoke(...) takes care of the actual invocation syntax.
//
//   ConverterGenerators -
//      An MPL iterator type over a sequence of metafunction classes
//      that can be applied to element 1...N of Sig to produce
//      argument from_python converters for the arguments
//
//   CallPolicies -
//      The precall, postcall, and what kind of resultconverter to
//      generate for mpl::front<Sig>::type
//
//   Sig -
//      The `intended signature' of the function. An MPL sequence
//      beginning with a result type and continuing with a list of
//      argument types.
template <class F, class ConverterGenerators, class CallPolicies, class Sig>
struct caller
    : caller_base_select<F,ConverterGenerators,CallPolicies,Sig>::type
{
    typedef typename caller_base_select<
        F,ConverterGenerators,CallPolicies,Sig
        >::type base;

    typedef PyObject* result_type;
    
    caller(F f, CallPolicies p) : base(f,p) {}
};


}}} // namespace boost::python::detail

# endif // CALLER_DWA20021121_HPP

#else

# define N BOOST_PP_ITERATION()

template <>
struct caller_arity<N>
{
    template <class F, class ConverterGenerators, class Policies, class Sig>
    struct impl
    {
        impl(F f, Policies p) : m_data(f,p) {}

        PyObject* operator()(PyObject* args_, PyObject*) // eliminate
                                                         // this
                                                         // trailing
                                                         // keyword dict
        {
            typedef typename mpl::begin<Sig>::type first;
            typedef typename first::type result_t;
            typedef typename select_result_converter<Policies, result_t>::type result_converter;
# if N
#  define BOOST_PP_LOCAL_MACRO(i) BOOST_PYTHON_ARG_CONVERTER(i)
#  define BOOST_PP_LOCAL_LIMITS (0, N-1)
#  include BOOST_PP_LOCAL_ITERATE()
# endif 
            // all converters have been checked. Now we can do the
            // precall part of the policy
            if (!m_data.second().precall(args_))
                return 0;

            typedef typename detail::invoke_tag<F>::type tag;

            PyObject* result = detail::invoke(
                tag(), result_converter(), m_data.first() BOOST_PP_ENUM_TRAILING_PARAMS(N, c));
            
            return m_data.second().postcall(args_, result);
        }
     private:
        compressed_pair<F,Policies> m_data;
    };
};



#endif // BOOST_PP_IS_ITERATING 


