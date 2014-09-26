// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef PYTYPE_ARG_FROM_PYTHON_DWA2002628_HPP
# define PYTYPE_ARG_FROM_PYTHON_DWA2002628_HPP

# include <boost/python/detail/wrap_python.hpp>

//
// arg_from_python converters for Python type wrappers, to be used as
// base classes for specializations.
//
namespace boost { namespace python { namespace converter { 

template <PyTypeObject* python_type>
struct pytype_arg_from_python
{
    pytype_arg_from_python(PyObject*);
    bool convertible() const;
 private:
    PyObject* m_src;
};

// rvalue converter base
template <class Wrapper, PyTypeObject* python_type>
struct pytype_wrapper_value_arg_from_python
    : pytype_arg_from_python<python_type>
{
    typedef Wrapper result_type;
    
    pytype_wrapper_value_arg_from_python(PyObject*);
    Wrapper operator()(PyObject*) const;
};

// Special case for Wrapper& - must store an lvalue internally. This
// OK because the entire state of the object is actually in the Python
// object.
template <class Wrapper, PyTypeObject* python_type>
struct pytype_wrapper_ref_arg_from_python
    : pytype_arg_from_python<python_type>
{
    typedef Wrapper& result_type;
    
    pytype_wrapper_ref_arg_from_python(PyObject*);
    Wrapper& operator()(PyObject*) const;
 private:
    mutable Wrapper m_result;
};

//
// implementations
//

template <PyTypeObject* python_type>
inline pytype_arg_from_python<python_type>::pytype_arg_from_python(PyObject* x)
    : m_src(x)
{
}

template <PyTypeObject* python_type>
inline bool pytype_arg_from_python<python_type>::convertible() const
{
    return PyObject_IsInstance(m_src, (PyObject*)python_type);
}

template <class Wrapper, PyTypeObject* python_type>
pytype_wrapper_value_arg_from_python<Wrapper,python_type>::pytype_wrapper_value_arg_from_python(
    PyObject* p)
    : pytype_arg_from_python<python_type>(p)
{
}

template <class Wrapper, PyTypeObject* python_type>
Wrapper pytype_wrapper_value_arg_from_python<Wrapper,python_type>::operator()(
    PyObject* x) const
{
    return Wrapper(python::detail::borrowed_reference(x));
}

template <class Wrapper, PyTypeObject* python_type>
pytype_wrapper_ref_arg_from_python<Wrapper,python_type>::pytype_wrapper_ref_arg_from_python(
    PyObject* p)
    : pytype_arg_from_python<python_type>(p)
      , m_result(python::detail::borrowed_reference(p))
{
}

template <class Wrapper, PyTypeObject* python_type>
Wrapper& pytype_wrapper_ref_arg_from_python<Wrapper,python_type>::operator()(
    PyObject* x) const
{
    return m_result;
}

}}} // namespace boost::python::converter

#endif // PYTYPE_ARG_FROM_PYTHON_DWA2002628_HPP
