// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef TYPE_ID_DWA2002517_HPP
# define TYPE_ID_DWA2002517_HPP

# include <boost/python/detail/wrap_python.hpp>
# include <boost/python/detail/config.hpp>
# include <boost/python/detail/msvc_typeinfo.hpp>
# include <boost/operators.hpp>
# include <typeinfo>
# include <cstring>
# include <boost/static_assert.hpp>
# include <boost/type_traits/same_traits.hpp>

namespace boost { namespace python { 

// for this compiler at least, cross-shared-library type_info
// comparisons don't work, so use typeid(x).name() instead. It's not
// yet clear what the best default strategy is.
# if (defined(__GNUC__) && __GNUC__ >= 3) \
 || defined(_AIX) \
 || (   defined(__sgi) && defined(__host_mips))
#  define BOOST_PYTHON_TYPE_ID_NAME
# endif 

// type ids which represent the same information as std::type_info
// (i.e. the top-level reference and cv-qualifiers are stripped), but
// which works across shared libraries.
struct type_info : private totally_ordered<type_info>
{
    inline type_info(std::type_info const& = typeid(void));
    
    inline bool operator<(type_info const& rhs) const;
    inline bool operator==(type_info const& rhs) const;

    char const* name() const;
    friend BOOST_PYTHON_DECL std::ostream& operator<<(
        std::ostream&, type_info const&);
    
 private: // data members
#  ifdef BOOST_PYTHON_TYPE_ID_NAME
    typedef char const* base_id_t;
#  else
    typedef std::type_info const* base_id_t;
#  endif
    
    base_id_t m_base_type;
};

template <class T>
inline type_info type_id(boost::type<T>* = 0)
{
    return type_info(
#  if (!defined(BOOST_MSVC) || BOOST_MSVC > 1300) && (!defined(BOOST_INTEL_CXX_VERSION) || BOOST_INTEL_CXX_VERSION > 700)
        typeid(T)
#  else // strip the decoration which msvc and Intel mistakenly leave in
        python::detail::msvc_typeid<T>()
#  endif 
        );
}

#  if defined(__EDG_VERSION__) && __EDG_VERSION__ < 245
// Older EDG-based compilers seems to mistakenly distinguish "int" from
// "signed int", etc., but only in typeid() expressions. However
// though int == signed int, the "signed" decoration is propagated
// down into template instantiations. Explicit specialization stops
// that from taking hold.

#   define BOOST_PYTHON_SIGNED_INTEGRAL_TYPE_ID(T)      \
template <>                                             \
inline type_info type_id<T>(boost::type<T>*)            \
{                                                       \
    return type_info(typeid(T));                        \
}

BOOST_PYTHON_SIGNED_INTEGRAL_TYPE_ID(short)
BOOST_PYTHON_SIGNED_INTEGRAL_TYPE_ID(int)
BOOST_PYTHON_SIGNED_INTEGRAL_TYPE_ID(long)
// using Python's macro instead of Boost's - we don't seem to get the
// config right all the time.
#   ifdef HAVE_LONG_LONG
BOOST_PYTHON_SIGNED_INTEGRAL_TYPE_ID(long long)
#   endif
#   undef BOOST_PYTHON_SIGNED_INTEGRAL_TYPE_ID
#  endif


inline type_info::type_info(std::type_info const& id)
    : m_base_type(
#  ifdef BOOST_PYTHON_TYPE_ID_NAME
        id.name()
#  else
        &id
#  endif
        )
{
}

inline bool type_info::operator<(type_info const& rhs) const
{
#  ifdef BOOST_PYTHON_TYPE_ID_NAME
    return std::strcmp(m_base_type, rhs.m_base_type) < 0;
#  else
    return m_base_type->before(*rhs.m_base_type);
#  endif 
}

inline bool type_info::operator==(type_info const& rhs) const
{
#  ifdef BOOST_PYTHON_TYPE_ID_NAME
    return !std::strcmp(m_base_type, rhs.m_base_type);
#  else
    return *m_base_type == *rhs.m_base_type;
#  endif 
}

inline char const* type_info::name() const
{
#  ifdef BOOST_PYTHON_TYPE_ID_NAME
    return m_base_type;
#  else
    return m_base_type->name();
#  endif 
}


BOOST_PYTHON_DECL std::ostream& operator<<(std::ostream&, type_info const&);

}} // namespace boost::python

#endif // TYPE_ID_DWA2002517_HPP
