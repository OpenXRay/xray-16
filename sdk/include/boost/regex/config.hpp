/*
 *
 * Copyright (c) 1998-2002
 * Dr John Maddock
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Dr John Maddock makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         config.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex extended config setup.
  */

#ifndef BOOST_REGEX_CONFIG_HPP
#define BOOST_REGEX_CONFIG_HPP
//
// Borland C++ Fix/error check
// this has to go *before* we include any std lib headers:
//
#if defined(__BORLANDC__)
#  if (__BORLANDC__ == 0x550) || (__BORLANDC__ == 0x551)
      // problems with std::basic_string and dll RTL:
#     if defined(_RTLDLL) && defined(_RWSTD_COMPILE_INSTANTIATE)
#        ifdef BOOST_REGEX_BUILD_DLL
#           error _RWSTD_COMPILE_INSTANTIATE must not be defined when building regex++ as a DLL
#        else
#           pragma message("Defining _RWSTD_COMPILE_INSTANTIATE when linking to the DLL version of the RTL may produce memory corruption problems in std::basic_string, as a result of separate versions of basic_string's static data in the RTL and you're exe/dll: be warned!!")
#        endif
#     endif
#     ifndef _RTLDLL
         // this is harmless for a staic link:
#        define _RWSTD_COMPILE_INSTANTIATE
#     endif
#  endif
#  if (__BORLANDC__ <= 0x540) && !defined(BOOST_REGEX_NO_LIB) && !defined(_NO_VCL)
      // C++ Builder 4 and earlier, we can't tell whether we should be using
      // the VCL runtime or not, do a static link instead:
#     define BOOST_REGEX_STATIC_LINK
#  endif
   //
   // VCL support:
   // if we're building a console app then there can't be any VCL (can there?)
#  if !defined(__CONSOLE__) && !defined(_NO_VCL)
#     define BOOST_REGEX_USE_VCL
#  endif
   //
   // if this isn't Win32 then don't automatically select link
   // libraries:
   //
#  ifndef _Windows
#     ifndef BOOST_REGEX_NO_LIB
#        define BOOST_REGEX_NO_LIB
#     endif
#     ifndef BOOST_REGEX_STATIC_LINK
#        define BOOST_REGEX_STATIC_LINK
#     endif
#  endif

#endif

/*****************************************************************************
 *
 *  Include all the headers we need here:
 *
 ****************************************************************************/

#ifdef __cplusplus

#  ifndef BOOST_REGEX_USER_CONFIG
#     define BOOST_REGEX_USER_CONFIG <boost/regex/user.hpp>
#  endif

#  include BOOST_REGEX_USER_CONFIG

#  include <cstdlib>
#  include <cstddef>
#  include <cstring>
#  include <cctype>
#  include <cstdio>
#  include <clocale>
#  include <cassert>
#  include <string>
#  include <stdexcept>
#  include <iterator>
#  include <boost/config.hpp>
#  include <boost/cstdint.hpp>
#  include <boost/detail/allocator.hpp>
#else
   //
   // C build,
   // don't include <boost/config.hpp> because that may
   // do C++ specific things in future...
   //
#  include <stdlib.h>
#  include <stddef.h>
#  ifdef _MSC_VER
#     define BOOST_MSVC _MSC_VER
#  endif
#endif

/*****************************************************************************
 *
 *  Boilerplate regex config options:
 *
 ****************************************************************************/

/* Obsolete macro, use BOOST_VERSION instead: */
#define BOOST_RE_VERSION 320

// fix:
#if defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

//
// If there isn't good enough wide character support then there will
// be no wide character regular expressions:
//
#if (defined(BOOST_NO_CWCHAR) || defined(BOOST_NO_CWCTYPE) || defined(BOOST_NO_STD_WSTRING))
#  if !defined(BOOST_NO_WREGEX)
#     define BOOST_NO_WREGEX
#  endif
#else
#  if defined(__sgi) && defined(__SGI_STL_PORT)
      // STLPort on IRIX is misconfigured: <cwctype> does not compile
      // as a temporary fix include <wctype.h> instead and prevent inclusion
      // of STLPort version of <cwctype>
#     include <wctype.h>
#     define __STLPORT_CWCTYPE
#     define _STLP_CWCTYPE
#  endif

#ifdef __cplusplus
#  include <cwchar>
#  include <cwctype>
#endif

#endif

//
// If Win32 support has been disabled for boost in general, then
// it is for regex in particular:
//
#ifdef BOOST_DISABLE_WIN32
#  define BOOST_REGEX_NO_W32
#endif

// some versions of gcc can't merge template instances:
#if defined(__CYGWIN__)
#  define BOOST_REGEX_NO_TEMPLATE_SWITCH_MERGE
#endif

// fix problems with bool as a macro,
// this probably doesn't affect any current compilers:
#if defined(bool) || defined(true) || defined(false)
#  define BOOST_REGEX_NO_BOOL
#endif

// We don't make our templates external if the compiler
// can't handle it:
#if (defined(BOOST_NO_MEMBER_FUNCTION_SPECIALIZATIONS) || defined(__HP_aCC) || defined(__MWERKS__) || defined(__COMO__) || defined(__ICL) || defined(__ICC))\
   && !defined(BOOST_MSVC) && !defined(__BORLANDC__)
#  define BOOST_REGEX_NO_EXTERNAL_TEMPLATES
#endif

// disable our own file-iterators and mapfiles if we can't
// support them:
#if !defined(BOOST_HAS_DIRENT_H) && !(defined(_WIN32) && !defined(BOOST_REGEX_NO_W32))
#  define BOOST_REGEX_NO_FILEITER
#endif

#ifdef __cplusplus
#ifndef MB_CUR_MAX
// yuk!
// better make a conservative guess!
#define MB_CUR_MAX 10
#endif

namespace boost{ namespace re_detail{
#ifdef BOOST_NO_STD_DISTANCE
template <class T>
std::ptrdiff_t distance(const T& x, const T& y)
{ return y - x; }
#else
using std::distance;
#endif
}}


#ifdef BOOST_REGEX_NO_BOOL
#  define BOOST_REGEX_MAKE_BOOL(x) static_cast<bool>((x) ? true : false)
#else
#  ifdef BOOST_MSVC
      // warning suppression with VC6:
#     pragma warning(disable: 4800)
#  endif
#  define BOOST_REGEX_MAKE_BOOL(x) static_cast<bool>(x)
#endif
#endif // __cplusplus

// backwards compatibitity:
#if defined(BOOST_RE_NO_LIB)
#  define BOOST_REGEX_NO_LIB
#endif

#if defined(__GNUC__) && (defined(_WIN32) || defined(__CYGWIN__))
// gcc on win32 has problems merging switch statements in templates:
#  define BOOST_REGEX_NO_TEMPLATE_SWITCH_MERGE
// gcc on win32 has problems if you include <windows.h>
// (sporadically generates bad code).
#  define BOOST_REGEX_USE_C_LOCALE
#  define BOOST_REGEX_NO_W32
#endif


/*****************************************************************************
 *
 *  Set up dll import/export options:
 *
 ****************************************************************************/

// backwards compatibility:
#ifdef BOOST_RE_STATIC_LIB
#  define BOOST_REGEX_STATIC_LINK
#endif

#if defined(BOOST_MSVC) && defined(_DLL)
#  define BOOST_REGEX_HAS_DLL_RUNTIME
#endif

#if defined(__BORLANDC__) && defined(_RTLDLL)
#  define BOOST_REGEX_HAS_DLL_RUNTIME
#endif

#if defined(__ICL) && defined(_DLL)
#  define BOOST_REGEX_HAS_DLL_RUNTIME
#endif

#if defined(BOOST_REGEX_HAS_DLL_RUNTIME) && !defined(BOOST_REGEX_STATIC_LINK)
#  if defined(BOOST_REGEX_SOURCE)
#     define BOOST_REGEX_DECL __declspec(dllexport)
#     define BOOST_REGEX_BUILD_DLL
#  else
#     define BOOST_REGEX_DECL __declspec(dllimport)
#  endif
#endif

#ifndef BOOST_REGEX_DECL
#  define BOOST_REGEX_DECL
#endif
 
#if (defined(BOOST_MSVC) || defined(__BORLANDC__)) && !defined(BOOST_REGEX_NO_LIB) && !defined(BOOST_REGEX_SOURCE)
#  include <boost/regex/v3/regex_library_include.hpp>
#endif

/*****************************************************************************
 *
 *  Set up function call type:
 *
 ****************************************************************************/

#if defined(BOOST_MSVC) || defined(__ICL)
#  if defined(_DEBUG)
#     define BOOST_REGEX_CALL __cdecl
#  else
#     define BOOST_REGEX_CALL __fastcall
#  endif
#  define BOOST_REGEX_CCALL __stdcall
#endif

#if defined(__BORLANDC__)
#  define BOOST_REGEX_CALL __fastcall
#  define BOOST_REGEX_CCALL __stdcall
#endif

#ifndef BOOST_REGEX_CALL
#  define BOOST_REGEX_CALL
#endif
#ifndef BOOST_REGEX_CCALL
#define BOOST_REGEX_CCALL
#endif

/*****************************************************************************
 *
 *  Set up localisation model:
 *
 ****************************************************************************/

// backwards compatibility:
#ifdef BOOST_RE_LOCALE_C
#  define BOOST_REGEX_USE_C_LOCALE
#endif

#ifdef BOOST_RE_LOCALE_CPP
#  define BOOST_REGEX_USE_CPP_LOCALE
#endif

// Win32 defaults to native Win32 locale:
#if defined(_WIN32) && !defined(BOOST_REGEX_USE_WIN32_LOCALE) && !defined(BOOST_REGEX_USE_C_LOCALE) && !defined(BOOST_REGEX_USE_CPP_LOCALE) && !defined(BOOST_REGEX_NO_W32)
#  define BOOST_REGEX_USE_WIN32_LOCALE
#endif
// otherwise use C locale:
#if !defined(BOOST_REGEX_USE_WIN32_LOCALE) && !defined(BOOST_REGEX_USE_C_LOCALE) && !defined(BOOST_REGEX_USE_CPP_LOCALE)
#  define BOOST_REGEX_USE_C_LOCALE
#endif

#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
#  include <windows.h>
#endif

#ifdef MAXPATH
#  define BOOST_REGEX_MAX_PATH MAXPATH
#elif defined(MAX_PATH)
#  define BOOST_REGEX_MAX_PATH MAX_PATH
#elif defined(FILENAME_MAX)
#  define BOOST_REGEX_MAX_PATH FILENAME_MAX
#else
#  define BOOST_REGEX_MAX_PATH 200
#endif



/*****************************************************************************
 *
 *  Error Handling for exception free compilers:
 *
 ****************************************************************************/

#ifdef BOOST_NO_EXCEPTIONS
//
// If there are no exceptions then we must report critical-errors
// the only way we know how; by terminating.
//
#ifdef __BORLANDC__
// <cstdio> seems not to make stderr usable with Borland:
#include <stdio.h>
#endif
#  define BOOST_REGEX_NOEH_ASSERT(x)\
if(0 == (x))\
{\
   std::fprintf(stderr, "Error: critical regex++ failure in \"%s\"", #x);\
   std::abort();\
}
#else
//
// With exceptions then error handling is taken care of and
// there is no need for these checks:
//
#  define BOOST_REGEX_NOEH_ASSERT(x)
#endif

/*****************************************************************************
 *
 *  Debugging / tracing support:
 *
 ****************************************************************************/

#if defined(BOOST_REGEX_DEBUG) && defined(__cplusplus)

#  include <iostream>
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;

#  ifndef jm_assert
#     define jm_assert(x) assert(x)
#  endif
#  ifndef jm_trace
#     define jm_trace(x) cerr << x << endl;
#  endif
#  ifndef jm_instrument
#     define jm_instrument jm_trace(__FILE__<<"#"<<__LINE__)
#  endif

namespace boost{
   namespace re_detail{
class debug_guard
{
public:
   char g1[32];
   const char* pc;
   char* pnc;
   const char* file;
   int line;
   char g2[32];
   debug_guard(const char* f, int l, const char* p1 = 0, char* p2 = 0);
   ~debug_guard();
};

#  define BOOST_RE_GUARD_STACK boost::re_detail::debug_guard sg(__FILE__, __LINE__);
#  define BOOST_RE_GUARD_GLOBAL(x) const char g1##x[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, }; char g2##x[32]; boost::debug_guard g3##x(__FILE__, __LINE__, g1##x, g2##x);

   } // namespace re_detail
} // namespace boost

#else

#  define jm_assert(x)
#  define jm_trace(x)
#  define BOOST_RE_GUARD_STACK
#  define BOOST_RE_GUARD_GLOBAL(x)
#  ifndef jm_instrument
#     define jm_instrument
#  endif
#endif

/*****************************************************************************
 *
 *  Fix broken compilers that wrongly #define some symbols:
 *
 ****************************************************************************/

#ifdef __cplusplus

#ifdef BOOST_NO_CTYPE_FUNCTIONS

// Make functions out of the macros.
// Use parentheses so the macros don't screw us up.
inline int (isalpha)(int c) { return isalpha(c); }
inline int (iscntrl)(int c) { return iscntrl(c); }
inline int (isdigit)(int c) { return isdigit(c); }
inline int (islower)(int c) { return islower(c); }
inline int (ispunct)(int c) { return ispunct(c); }
inline int (isspace)(int c) { return isspace(c); }
inline int (isupper)(int c) { return isupper(c); }
inline int (isxdigit)(int c) { return isxdigit(c); }

#endif

// the following may be defined as macros; this is
// incompatable with std::something syntax, we have
// no choice but to undef them?

#ifdef memcpy
#undef memcpy
#endif
#ifdef memmove
#undef memmove
#endif
#ifdef memset
#undef memset
#endif
#ifdef sprintf
#undef sprintf
#endif
#ifdef strcat
#undef strcat
#endif
#ifdef strcmp
#undef strcmp
#endif
#ifdef strcpy
#undef strcpy
#endif
#ifdef strlen
#undef strlen
#endif
#ifdef swprintf
#undef swprintf
#endif
#ifdef wcslen
#undef wcslen
#endif
#ifdef wcscpy
#undef wcscpy
#endif
#ifdef wcscmp
#undef wcscmp
#endif
#ifdef isalpha
#undef isalpha
#endif
#ifdef iscntrl
#undef iscntrl
#endif
#ifdef isdigit
#undef isdigit
#endif
#ifdef islower
#undef islower
#endif
#ifdef isupper
#undef isupper
#endif
#ifdef ispunct
#undef ispunct
#endif
#ifdef isspace
#undef isspace
#endif
#ifdef isxdigit
#undef isxdigit
#endif

#ifdef tolower
#undef tolower
#endif
#ifdef iswalpha
#undef iswalpha
#endif
#ifdef iswcntrl
#undef iswcntrl
#endif
#ifdef iswdigit
#undef iswdigit
#endif
#ifdef iswlower
#undef iswlower
#endif
#ifdef iswpunct
#undef iswpunct
#endif
#ifdef iswspace
#undef iswspace
#endif
#ifdef iswupper
#undef iswupper
#endif
#ifdef iswxdigit
#undef iswxdigit
#endif
#ifdef towlower
#undef towlower
#endif
#ifdef wcsxfrm
#undef wcsxfrm
#endif

#endif

/*****************************************************************************
 *
 *  Fix broken broken namespace support:
 *
 ****************************************************************************/

#if defined(BOOST_NO_STDC_NAMESPACE) && defined(__cplusplus)

namespace std{
   using ::ptrdiff_t;
   using ::size_t;
   using ::memcpy;
   using ::memmove;
   using ::memset;
   using ::memcmp;
   using ::sprintf;
   using ::strcat;
   using ::strcmp;
   using ::strcpy;
   using ::strlen;
   using ::strxfrm;
   using ::isalpha;
   using ::iscntrl;
   using ::isdigit;
   using ::islower;
   using ::isupper;
   using ::ispunct;
   using ::isspace;
   using ::isxdigit;
   using ::tolower;
   using ::abs;
   using ::setlocale;
#  ifndef BOOST_NO_WREGEX
#     ifndef BOOST_NO_SWPRINTF
   using ::swprintf;
#     endif
   using ::wcslen;
   using ::wcscpy;
   using ::wcscmp;
   using ::iswalpha;
   using ::iswcntrl;
   using ::iswdigit;
   using ::iswlower;
   using ::iswpunct;
   using ::iswspace;
   using ::iswupper;
   using ::iswxdigit;
   using ::towlower;
   using ::wcsxfrm;
   using ::wcstombs;
   using ::mbstowcs;
#     if !defined(BOOST_NO_STD_LOCALE) && !defined (__STL_NO_NATIVE_MBSTATE_T) && !defined(_STLP_NO_NATIVE_MBSTATE_T)
   using ::mbstate_t;
#     endif
#  endif // BOOST_NO_WREGEX
   using ::fseek;
   using ::fread;
   using ::ftell;
   using ::fopen;
   using ::fclose;
   using ::FILE;
#ifdef BOOST_NO_EXCEPTIONS
   using ::fprintf;
   using ::abort;
#endif
}

#endif

/*****************************************************************************
 *
 *  helper functions pointer_construct/pointer_destroy:
 *
 ****************************************************************************/

#ifdef __cplusplus
namespace boost{ namespace re_detail{

#ifdef BOOST_MSVC
#pragma warning (push)
#pragma warning (disable : 4100)
#endif

template <class T>
inline void pointer_destroy(T* p)
{ p->~T(); (void)p; }

#ifdef BOOST_MSVC
#pragma warning (pop)
#endif

template <class T>
inline void pointer_construct(T* p, const T& t)
{ new (p) T(t); }

}} // namespaces
#endif

#endif









