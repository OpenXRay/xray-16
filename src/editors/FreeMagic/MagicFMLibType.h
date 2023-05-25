// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

#ifndef MAGICFMLIBTYPE_H
#define MAGICFMLIBTYPE_H

// Windows platform
#ifdef WIN32

// For the DLL library.
#ifdef MAGICFMDLL_EXPORTS
#define MAGICFM __declspec(dllexport)

// For a client of the DLL library.
#else
#ifdef MAGICFMDLL_IMPORTS
#define MAGICFM __declspec(dllimport)

// For the static library.
#else
#define MAGICFM

#endif
#endif

// Disable warning C4251.  Template classes cannot be exported for the obvious
// reason that the code is not generated until an instance of the class is
// declared.  With this warning enabled, you get many complaints about class
// data members that are of template type.
//
// When developing the DLL code, you should enable the warning to catch places
// where you should have used MAGICFM.  For example, nested classes and friend
// functions should be tagged with this macro.
#pragma warning(disable : 4251)

// Disable warning C4275:  "non dll-interface class 'someClass' used as base
// for dll-interface class 'anotherClass'.  The only time this warning occurs
// in the DLL projects is when compiling MgcBinary2D, a class derived from
// a template class.  The DLL library works fine, so I disabled the warning.
// You might want to enable it if you want to catch base classes that are
// non-template and non-dll-interface when the derived class is dll-interface.
#pragma warning(disable : 4275)

// Disable warning C4514.  "unreferenced inline function has been removed"
// This occurs a lot when you increase the warning level to 4.
#pragma warning(disable : 4514)

// Disable warning C4127:  "conditional expression is constant"
// A few loops are controlled by "while (true) { 'break' somewhere }".
#pragma warning(disable : 4127)

// Disable warning C4097:  "typedef-name 'someType' used as synonym for
// class-name 'someTemplateType'.  Occurs when a base class for a derived
// class is a typedef'd name for a template type.  This only occurs when
// compiling MgcBinary2D.
#pragma warning(disable : 4097)

// Disable the warning about truncating the debug names to 255 characters.
// This warning shows up often with STL code.
#pragma warning(disable : 4786)

// Warning C4702: "unreachable code".  This is incorrectly generated at
// Level 4 warnings in Release builds in the following situation:
//
// ret_type Function ()
// {
//     if ( condition )
//         return a_value;
//     else
//         return another_value;
// }  <-- line at which the compiler complains

// Linux platform
#else
#define MAGICFM
#endif

#endif
