//  (C) Copyright Boost.org 2001. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version.

//  Win32 specific config options:

#define BOOST_PLATFORM "Win32"

#if defined(__GNUC__) && !defined(BOOST_NO_SWPRINTF)
#  define BOOST_NO_SWPRINTF
#endif

#ifndef BOOST_DISABLE_WIN32
//
// Win32 will normally be using native Win32 threads,
// but there is a pthread library avaliable as an option:
//
#ifndef BOOST_HAS_PTHREADS
#  define BOOST_HAS_WINTHREADS
#endif

// WEK: Added
#define BOOST_HAS_FTIME

#endif

//
// disable min/max macros:
//
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif

#ifdef BOOST_MSVC
namespace std{
  // Apparently, something in the Microsoft libraries requires the "long"
  // overload, because it calls the min/max functions with arguments of
  // slightly different type.  (If this proves to be incorrect, this
  // whole "BOOST_MSVC" section can be removed.)
  inline long min(long __a, long __b) {
    return __b < __a ? __b : __a;
  }
  inline long max(long __a, long __b) {
    return  __a < __b ? __b : __a;
  }
  // The "long double" overload is required, otherwise user code calling
  // min/max for floating-point numbers will use the "long" overload.
  // (SourceForge bug #495495)
  inline long double min(long double __a, long double __b) {
    return __b < __a ? __b : __a;
  }
  inline long double max(long double __a, long double __b) {
    return  __a < __b ? __b : __a;
  }
}
using std::min;
using std::max;
#     endif
