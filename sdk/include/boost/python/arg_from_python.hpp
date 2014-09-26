// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef ARG_FROM_PYTHON_DWA2002128_HPP
# define ARG_FROM_PYTHON_DWA2002128_HPP

# include <boost/python/converter/arg_from_python.hpp>
# include <boost/python/detail/indirect_traits.hpp>

namespace boost { namespace python { 

template <class T>
struct arg_from_python
    : converter::select_arg_from_python<T>::type
{
    typedef typename converter::select_arg_from_python<T>::type base;
    arg_from_python(PyObject*);
};

// specialization for PyObject*
template <>
struct arg_from_python<PyObject*>
{
    typedef PyObject* result_type;
    
    arg_from_python(PyObject*) {}
    bool convertible() const { return true; }
    PyObject* operator()(PyObject* source) const { return source; }
};

template <>
struct arg_from_python<PyObject* const&>
{
    typedef PyObject* const& result_type;
    arg_from_python(PyObject*) {}
    bool convertible() const { return true; }
    PyObject*const& operator()(PyObject*const& source) const { return source; }
};

namespace detail
{
  //
  // Meta-iterators for use with caller<>
  //
  
  // temporary hack
  template <class T> struct nullary : T
  {
      nullary(PyObject* x) : T(x), m_p(x) {}
      typename T::result_type operator()() { return this->T::operator()(m_p); }
      PyObject* m_p;
  };

  // An MPL metafunction class which returns arg_from_python<ArgType>
  struct gen_arg_from_python
  {
      template <class ArgType> struct apply
      {
          typedef nullary<arg_from_python<ArgType> > type;
      };
  };

  // An MPL iterator over an endless sequence of gen_arg_from_python
  struct args_from_python
  {
      typedef gen_arg_from_python type;
      typedef args_from_python next;
  };
}

//
// implementations
//
template <class T>
inline arg_from_python<T>::arg_from_python(PyObject* source)
    : base(source)
{
}

}} // namespace boost::python

#endif // ARG_FROM_PYTHON_DWA2002128_HPP
