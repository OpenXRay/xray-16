// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLSYSTEMH
#define WMLSYSTEMH

#define WIN32

// Microsoft Windows
#if defined(WIN32)
#include "WmlWinSystem.h"

// Macintosh OS X
#elif defined(__APPLE__)
#include "WmlMacSystem.h"

// SGI IRIX
#elif defined(SGI_IRIX)
#include "WmlIrixSystem.h"

// HP-UX
#elif defined(HP_UX)
#include "WmlHpuxSystem.h"

// Sun SOLARIS
#elif defined(SUN_SOLARIS)
#include "WmlSolarisSystem.h"

// Linux on a PC. Red Hat 8.x g++ has problems with specialized instantiation
// of static members in template classes *before* the class itself is
// explicitly instantiated.  The problem is not consistent; for example, Math
// Vector*, and Matrix* classes compile fine, but not Integrate1 or
// BSplineRectangle.
#else
#include "WmlLnxSystem.h"

#endif

namespace Wml
{

class WML_ITEM System
{
public:
    // little/big endian support
    static void SwapBytes (int iSize, void* pvValue);
    static void SwapBytes (int iSize, int iQuantity, void* pvValue);
    static void EndianCopy (int iSize, const void* pvSrc, void* pvDst);
    static void EndianCopy (int iSize, int iQuantity, const void* pvSrc,
        void* pvDst);

    static unsigned int MakeRGB (unsigned char ucR, unsigned char ucG,
        unsigned char ucB);

    static unsigned int MakeRGBA (unsigned char ucR, unsigned char ucG,
        unsigned char ucB, unsigned char ucA);

    // time utilities
    static double GetTime ();

    // TO DO.  Pathname handling to access files in subdirectories.
    static bool FileExists (const char* acFilename);

    // convenient utilities
    static bool IsPowerOfTwo (int iValue);
};

// allocation and deallocation of 2D arrays
template <class T> void Allocate2D (int iCols, int iRows, T**& raatArray);
template <class T> void Deallocate2D (T** aatArray);

// allocation and deallocation of 3D arrays
template <class T> void Allocate3D (int iCols, int iRows, int iSlices,
    T***& raaatArray);
template <class T> void Deallocate3D (int iRows, int iSlices,
    T*** aaatArray);

#include "WmlSystem.inl"
#include "WmlSystem.mcr"

}

#endif

