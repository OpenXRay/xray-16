// (C) 2003, Fernando Luis Cacciola Carballal.
//
// This material is provided "as is", with absolutely no warranty expressed
// or implied. Any use is at your own risk.
//
// Permission to use or copy this software for any purpose is hereby granted
// without fee, provided the above notices are retained on all copies.
// Permission to modify the code and to distribute modified code is granted,
// provided the above notices are retained, and a notice that the code was
// modified is included with the above copyright notice.
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//
#ifndef BOOST_OPTIONAL_FLC_19NOV2002_HPP
#define BOOST_OPTIONAL_FLC_19NOV2002_HPP

#include<new>
#include<algorithm>

#include "boost/config.hpp"
#include "boost/assert.hpp"
#include "boost/type_traits/alignment_of.hpp"
#include "boost/type_traits/type_with_alignment.hpp"

#if BOOST_WORKAROUND(BOOST_MSVC, == 1200)
// VC6.0 has the following bug:
//   When a templated assignment operator exist, an implicit conversion
//   constructing an optional<T> is used when assigment of the form:
//     optional<T> opt ; opt = T(...);
//   is compiled.
//   However, optional's ctor is _explicit_ and the assignemt shouldn't compile.
//   Therefore, for VC6.0 templated assignment is disabled.
//
#define BOOST_OPTIONAL_NO_CONVERTING_ASSIGNMENT
#endif

#if BOOST_WORKAROUND(BOOST_MSVC, == 1300)
// VC7.0 has the following bug:
//   When both a non-template and a template copy-ctor exist
//   and the templated version is made 'explicit', the explicit is also
//   given to the non-templated version, making the class non-implicitely-copyable.
//
#define BOOST_OPTIONAL_NO_CONVERTING_COPY_CTOR
#endif

namespace boost
{
  namespace optional_detail
  {
    template <class T>
    class aligned_storage
    {
         // Borland ICEs if unnamed unions are used for this!
         union dummy_u
         {
             char data[ sizeof(T) ];
             BOOST_DEDUCED_TYPENAME type_with_alignment<
               ::boost::alignment_of<T>::value >::type aligner_;
         } dummy_ ;

      public:

        void const* address() const { return &dummy_.data[0]; }
        void      * address()       { return &dummy_.data[0]; }
    } ;
  }

template<class T>
class optional
{
    typedef optional<T> this_type ;

    typedef optional_detail::aligned_storage<T> storage_type ;

    typedef void (this_type::*unspecified_bool_type)();
    
  public :

    typedef T value_type ;

    // Creates an optional<T> uninitialized.
    // No-throw
    optional ()
      :
      m_initialized(false) {}

    // Creates an optional<T> initialized with 'val'.
    // Can throw if T::T(T const&) does
    explicit optional ( T const& val )
      :
      m_initialized(false)
    {
      construct(val);
    }

#ifndef BOOST_OPTIONAL_NO_CONVERTING_COPY_CTOR
    // NOTE: MSVC needs templated versions first

    // Creates a deep copy of another convertible optional<U>
    // Requires a valid conversion from U to T.
    // Can throw if T::T(U const&) does
    template<class U>
    explicit optional ( optional<U> const& rhs )
      :
      m_initialized(false)
    {
      if ( rhs )
        construct(*rhs);
    }
#endif

    // Creates a deep copy of another optional<T>
    // Can throw if T::T(T const&) does
    optional ( optional const& rhs )
      :
      m_initialized(false)
    {
      if ( rhs )
        construct(*rhs);
    }

    // No-throw (assuming T::~T() doesn't)
    ~optional() { destroy() ; }

#ifndef BOOST_OPTIONAL_NO_CONVERTING_ASSIGNMENT
    // Assigns from another convertible optional<U> (converts && deep-copies the rhs value)
    // Requires a valid conversion from U to T.
    // Basic Guarantee: If T::T( U const& ) throws, this is left UNINITIALIZED
    template<class U>
    optional& operator= ( optional<U> const& rhs )
      {
        destroy(); // no-throw

        if ( rhs )
        {
          // An exception can be thrown here.
          // It it happens, THIS will be left uninitialized.
          construct(*rhs);
        }
        return *this ;
      }
#endif

    // Assigns from another optional<T> (deep-copies the rhs value)
    // Basic Guarantee: If T::T( T const& ) throws, this is left UNINITIALIZED
    optional& operator= ( optional const& rhs )
      {
        destroy(); // no-throw

        if ( rhs )
        {
          // An exception can be thrown here.
          // It it happens, THIS will be left uninitialized.
          construct(*rhs);
        }
        return *this ;
      }

    // Destroys the current value, if any, leaving this UNINITIALIZED
    // No-throw (assuming T::~T() doesn't)
    void reset()
      {
        destroy();
      }

    // Replaces the current value -if any- with 'val'
    // Basic Guarantee: If T::T( T const& ) throws this is left UNINITIALIZED.
    void reset ( T const& val )
      {
        destroy();
        construct(val);
      }

    // Returns a pointer to the value if this is initialized, otherwise,
    // returns NULL.
    // No-throw
    T const* get() const { return m_initialized ? static_cast<T const*>(m_storage.address()) : 0 ; }
    T*       get()       { return m_initialized ? static_cast<T*>      (m_storage.address()) : 0 ; }

    // Returns a pointer to the value if this is initialized, otherwise,
    // the behaviour is UNDEFINED
    // No-throw
    T const* operator->() const { BOOST_ASSERT(m_initialized) ; return static_cast<T const*>(m_storage.address()) ; }
    T*       operator->()       { BOOST_ASSERT(m_initialized) ; return static_cast<T*>      (m_storage.address()) ; }

    // Returns a reference to the value if this is initialized, otherwise,
    // the behaviour is UNDEFINED
    // No-throw
    T const& operator *() const { BOOST_ASSERT(m_initialized) ; return *static_cast<T const*>(m_storage.address()) ; }
    T&       operator *()       { BOOST_ASSERT(m_initialized) ; return *static_cast<T*>      (m_storage.address()) ; }

    // implicit conversion to "bool"
    // No-throw
    operator unspecified_bool_type() const { return m_initialized ? &this_type::destroy : 0 ; }

       // This is provided for those compilers which don't like the conversion to bool
       // on some contexts.
       bool operator!() const { return !m_initialized ; }

  private :

    void construct ( T const& val )
     {
       new (m_storage.address()) T(val) ;
       m_initialized = true ;
     }

    void destroy()
     {
       if ( m_initialized )
       {
         get()->~T() ;
         m_initialized = false ;
       }
     }

    bool m_initialized ;
    storage_type m_storage ;
} ;

// Returns a pointer to the value if this is initialized, otherwise, returns NULL.
// No-throw
template<class T>
inline
T const* get_pointer ( optional<T> const& opt )
{
  return opt.get() ;
}

template<class T>
inline
T* get_pointer ( optional<T>& opt )
{
  return opt.get() ;
}

// template<class OP> bool equal_pointees(OP const& x, OP const& y);
//
// Being OP a model of OptionalPointee (either a pointer or an optional):
//
// If both x and y have valid pointees, returns the result of (*x == *y)
// If only one has a valid pointee, returns false.
// If none have valid pointees, returns true.
// No-throw
template<class OptionalPointee>
inline
bool equal_pointees ( OptionalPointee const& x, OptionalPointee const& y )
{
  return (!x) != (!y) ? false : ( !x ? true : (*x) == (*y) ) ;
}

// optional's operator == and != have deep-semantics (compare values).
// WARNING: This is UNLIKE pointers. Use equal_pointees() in generic code instead.
template<class T>
inline
bool operator == ( optional<T> const& x, optional<T> const& y )
{ return equal_pointees(x,y); }

template<class T>
inline
bool operator != ( optional<T> const& x, optional<T> const& y )
{ return !( x == y ) ; }


//
// The following swap implementation follows the GCC workaround as found in
//  "boost/detail/compressed_pair.hpp"
//
namespace optional_detail {

#ifdef __GNUC__
   // workaround for GCC (JM):
   using std::swap;
#endif

// optional's swap:
// If both are initialized, calls swap(T&, T&), with whatever exception guarantess are given there.
// If only one is initialized, calls I.reset() and U.reset(*I), with the Basic Guarantee
// If both are uninitialized, do nothing (no-throw)
template<class T>
inline
void optional_swap ( optional<T>& x, optional<T>& y )
{
  if ( !x && !!y )
  {
    x.reset(*y); // Basic guarantee.
    y.reset();
  }
  else if ( !!x && !y )
  {
    y.reset(*x); // Basic guarantee.
    x.reset();
  }
  else if ( !!x && !!y )
  {
#ifndef __GNUC__
    using std::swap ;
#endif
    swap(*x,*y);
  }
}

} // namespace optional_detail

template<class T> inline void swap ( optional<T>& x, optional<T>& y )
{
  optional_detail::optional_swap(x,y);
}


} // namespace boost

#endif

