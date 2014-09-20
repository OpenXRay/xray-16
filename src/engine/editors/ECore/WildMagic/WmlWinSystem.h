// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLWINSYSTEMH
#define WMLWINSYSTEMH

// for a DLL library
#if defined(WML_DLL_EXPORT)
#define WML_ITEM __declspec(dllexport)

// for a client of the DLL library
#elif defined(WML_DLL_IMPORT)
#define WML_ITEM __declspec(dllimport)

// for a static library
#else
#define WML_ITEM

#endif

#if defined(_MSC_VER)

// Microsoft Visual C++ specific pragmas.  MSVC6 appears to be version 1200
// and MSVC7 appears to be version 1300.
#if _MSC_VER < 1300
#define WML_USING_VC6
#else
#define WML_USING_VC7
#endif

#if defined(WML_USING_VC6)

// Disable the warning about truncating the debug names to 255 characters.
// This warning shows up often with STL code in MSVC6, but not MSVC7.
#pragma warning( disable : 4786 )

// This warning is disabled because MSVC6 warns about not finding
// implementations for the pure virtual functions that occur in the template
// classes 'template <class Real>' when explicity instantiating the classe.
// NOTE:  If you create your own template classes that will be explicitly
// instantiated, you should re-enable the warning to make sure that in fact
// all your member data and functions have been defined and implemented.
#pragma warning( disable : 4661 )

#endif

// TO DO.  What does this warning mean?
// warning C4251:  class 'std::vector<_Ty,_Ax>' needs to have dll-interface
//   to be used by clients of class 'foobar'
#pragma warning( disable : 4251 )

#endif

// Specialized instantiation of static members in template classes before or
// after the class itself is instantiated is not a problem with Visual Studio
// .NET 2003 (VC 7.1), but VC 6 likes the specialized instantiation to occur
// after the class instantiation.
// #define WML_INSTANTIATE_BEFORE

// common standard library headers
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

#endif
