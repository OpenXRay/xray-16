// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

#ifndef WMLSYSTEM_MCR
#define WMLSYSTEM_MCR

// Fast conversion from a IEEE 32-bit floating point number F in [0,1] to a
// a 32-bit integer I in [0,2^L-1].
//
//   fFloat = F
//   iLog = L
//   iInt = I

#define WmlScaledFloatToInt(fFloat,iLog,iInt)\
{ \
    int iShift = 150 - iLog - ((*(int*)(&fFloat) >> 23) & 0xFF); \
    if ( iShift < 24 ) \
    { \
        iInt = ((*(int*)(&fFloat) & 0x007FFFFF) | \
            0x00800000) >> iShift; \
        if ( iInt == (1 << iLog) ) \
        { \
            iInt--; \
        } \
    } \
    else \
    { \
        iInt = 0; \
    } \
}

#endif
