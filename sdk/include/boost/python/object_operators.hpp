// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef OBJECT_OPERATORS_DWA2002617_HPP
# define OBJECT_OPERATORS_DWA2002617_HPP

# include <boost/python/object_core.hpp>
# include <boost/python/call.hpp>

namespace boost { namespace python { namespace api {

template <class U>
object object_operators<U>::operator()() const
{
    object_cref2 f = *static_cast<U const*>(this);
    return call<object>(f.ptr());
}


template <class U>
inline
object_operators<U>::operator bool_type() const
{
    object_cref2 x = *static_cast<U const*>(this);
    return PyObject_IsTrue(x.ptr()) ? &object::ptr : 0;
}

template <class U>
inline bool
object_operators<U>::operator!() const
{
    object_cref2 x = *static_cast<U const*>(this);
    return !PyObject_IsTrue(x.ptr());
}

# define BOOST_PYTHON_COMPARE_OP(op, opid)                      \
template <class L, class R>                                     \
bool operator op(L const& l, R const& r)                        \
{                                                               \
    return PyObject_RichCompareBool(                            \
        object(l).ptr(), object(r).ptr(), opid);                \
}
BOOST_PYTHON_COMPARE_OP(>, Py_GT)
BOOST_PYTHON_COMPARE_OP(>=, Py_GE)
BOOST_PYTHON_COMPARE_OP(<, Py_LT)
BOOST_PYTHON_COMPARE_OP(<=, Py_LE)
BOOST_PYTHON_COMPARE_OP(==, Py_EQ)
BOOST_PYTHON_COMPARE_OP(!=, Py_NE)
# undef BOOST_PYTHON_COMPARE_OP
    
# define BOOST_PYTHON_BINARY_OPERATOR(op)                               \
BOOST_PYTHON_DECL object operator op(object const& l, object const& r); \
template <class L, class R>                                             \
object operator op(L const& l, R const& r)                              \
{                                                                       \
    return object(l) op object(r);                                      \
}
BOOST_PYTHON_BINARY_OPERATOR(+)
BOOST_PYTHON_BINARY_OPERATOR(-)
BOOST_PYTHON_BINARY_OPERATOR(*)
BOOST_PYTHON_BINARY_OPERATOR(/)
BOOST_PYTHON_BINARY_OPERATOR(%)
BOOST_PYTHON_BINARY_OPERATOR(<<)
BOOST_PYTHON_BINARY_OPERATOR(>>)
BOOST_PYTHON_BINARY_OPERATOR(&)
BOOST_PYTHON_BINARY_OPERATOR(^)
BOOST_PYTHON_BINARY_OPERATOR(|)
# undef BOOST_PYTHON_BINARY_OPERATOR

        
# define BOOST_PYTHON_INPLACE_OPERATOR(op)                              \
BOOST_PYTHON_DECL object& operator op(object& l, object const& r);      \
template <class R>                                                      \
object& operator op(object& l, R const& r)                              \
{                                                                       \
    return l op object(r);                                              \
}
BOOST_PYTHON_INPLACE_OPERATOR(+=)
BOOST_PYTHON_INPLACE_OPERATOR(-=)
BOOST_PYTHON_INPLACE_OPERATOR(*=)
BOOST_PYTHON_INPLACE_OPERATOR(/=)
BOOST_PYTHON_INPLACE_OPERATOR(%=)
BOOST_PYTHON_INPLACE_OPERATOR(<<=)
BOOST_PYTHON_INPLACE_OPERATOR(>>=)
BOOST_PYTHON_INPLACE_OPERATOR(&=)
BOOST_PYTHON_INPLACE_OPERATOR(^=)
BOOST_PYTHON_INPLACE_OPERATOR(|=)
# undef BOOST_PYTHON_INPLACE_OPERATOR

}}} // namespace boost::python

#endif // OBJECT_OPERATORS_DWA2002617_HPP
