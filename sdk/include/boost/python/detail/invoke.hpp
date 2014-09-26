#if !defined(BOOST_PP_IS_ITERATING)

// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
# ifndef INVOKE_DWA20021122_HPP
#  define INVOKE_DWA20021122_HPP

#  include <boost/python/detail/wrap_python.hpp>
#  include <boost/python/detail/preprocessor.hpp>
#  include <boost/python/detail/none.hpp>

#  include <boost/type_traits/is_member_function_pointer.hpp>

#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/facilities/intercept.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#  include <boost/preprocessor/repetition/enum_binary_params.hpp>
#  include <boost/python/to_python_value.hpp>

// This file declares a series of overloaded invoke(...)  functions,
// used to invoke wrapped C++ function (object)s from Python. Each one
// accepts:
//
//   - a tag which identifies the invocation syntax (e.g. member
//   functions must be invoked with a different syntax from regular
//   functions)
//
//   - a pointer to a result converter type, used solely as a way of
//   transmitting the type of the result converter to the function (or
//   an int, if the return type is void).
//
//   - the "function", which may be a function object, a function or
//   member function pointer, or a defaulted_virtual_fn.
//
//   - The arg_from_python converters for each of the arguments to be
//   passed to the function being invoked.

namespace boost { namespace python { namespace detail { 

// This "result converter" is really just used as a dispatch tag to
// invoke(...), selecting the appropriate implementation
typedef int void_result_to_python;

// Trait forward declaration.
template <class T> struct is_defaulted_virtual_fn;

// Tag types describing invocation methods
struct fn_tag {};
struct mem_fn_tag {};

// A metafunction returning the appropriate tag type for invoking an
// object of type T.
template <class T>
struct invoke_tag
    : mpl::if_<
        is_member_function_pointer<T>
        , mem_fn_tag
        , fn_tag
    >
{};

#  define BOOST_PP_ITERATION_PARAMS_1                                            \
        (3, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/detail/invoke.hpp>))
#  include BOOST_PP_ITERATE()

}}} // namespace boost::python::detail

# endif // INVOKE_DWA20021122_HPP
#else 

# define N BOOST_PP_ITERATION()

template <class RC, class F BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class AC)>
inline PyObject* invoke(fn_tag, RC*, F& f BOOST_PP_ENUM_TRAILING_BINARY_PARAMS_Z(1, N, AC, & ac) )
{
    return RC()(f( BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, ac, () BOOST_PP_INTERCEPT) ));
}
                 
template <class F BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class AC)>
inline PyObject* invoke(fn_tag, void_result_to_python, F& f BOOST_PP_ENUM_TRAILING_BINARY_PARAMS_Z(1, N, AC, & ac) )
{
    f( BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, ac, () BOOST_PP_INTERCEPT) );
    return none();
}

template <class RC, class F, class TC BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class AC)>
inline PyObject* invoke(mem_fn_tag, RC*, F& f, TC& tc BOOST_PP_ENUM_TRAILING_BINARY_PARAMS_Z(1, N, AC, & ac) )
{
    return RC()( (tc().*f)(BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, ac, () BOOST_PP_INTERCEPT)) );
}
                 
template <class F, class TC BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class AC)>
inline PyObject* invoke(mem_fn_tag, void_result_to_python, F& f, TC& tc BOOST_PP_ENUM_TRAILING_BINARY_PARAMS_Z(1, N, AC, & ac) )
{
    (tc().*f)(BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, ac, () BOOST_PP_INTERCEPT));
    return none();
}

# undef N

#endif // BOOST_PP_IS_ITERATING 
