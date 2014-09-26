// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef PY_FUNCTION_DWA200286_HPP
# define PY_FUNCTION_DWA200286_HPP
# include <boost/function/function2.hpp>

namespace boost { namespace python { namespace objects {

// This type is used as a "generalized Python callback", wrapping the
// function signature:
//
//      PyObject* (PyObject* args, PyObject* keywords)
//
// We use boost::function to avoid generating lots of virtual tables
typedef boost::function2<PyObject*, PyObject*, PyObject*> py_function;

}}} // namespace boost::python::objects

#endif // PY_FUNCTION_DWA200286_HPP
