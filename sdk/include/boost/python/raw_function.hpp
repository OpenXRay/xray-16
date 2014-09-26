// Copyright David Abrahams 2003. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef RAW_FUNCTION_DWA200336_HPP
# define RAW_FUNCTION_DWA200336_HPP

# include <boost/python/tuple.hpp>
# include <boost/python/dict.hpp>
# include <boost/python/object/py_function.hpp>

# include <cstddef>

namespace boost { namespace python { 

namespace detail
{
  template <class F>
  struct raw_dispatcher
  {
      raw_dispatcher(F f) : f(f) {}
      
      PyObject* operator()(PyObject* args, PyObject* keywords)
      {
          return incref(
              object(
                  f(tuple(borrowed_reference(args)), dict(borrowed_reference(keywords)))
              ).ptr()
          );
      }
   private:
      F f;
  };

  object BOOST_PYTHON_DECL make_raw_function(objects::py_function, std::size_t min_args);
}

template <class F>
object raw_function(F f, std::size_t min_args = 0)
{
    return detail::make_raw_function(detail::raw_dispatcher<F>(f), min_args);
}
    
}} // namespace boost::python

#endif // RAW_FUNCTION_DWA200336_HPP
