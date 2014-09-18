// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.
#include "stdafx.h"
#pragma hdrstop

#include "WmlSystem.h"
using namespace Wml;

//----------------------------------------------------------------------------
void Wml::System::EndianCopy (int iSize, const void* pvSrc, void* pvDst)
{
    memcpy(pvDst,pvSrc,iSize);
}
//----------------------------------------------------------------------------
void Wml::System::EndianCopy (int iSize, int iQuantity, const void* pvSrc,
    void* pvDst)
{
    memcpy(pvDst,pvSrc,iSize*iQuantity);
}
//----------------------------------------------------------------------------
unsigned int Wml::System::MakeRGB (unsigned char ucR, unsigned char ucG,
    unsigned char ucB)
{
    return (ucR | (ucG << 8) | (ucB << 16) | (0xFF << 24));
}
//----------------------------------------------------------------------------
unsigned int Wml::System::MakeRGBA (unsigned char ucR, unsigned char ucG,
    unsigned char ucB, unsigned char ucA)
{
    return (ucR | (ucG << 8) | (ucB << 16) | (ucA << 24));
}
//----------------------------------------------------------------------------
double Wml::System::GetTime ()
{
    // 64-bit quantities
    LARGE_INTEGER iFrequency, iCounter;

    QueryPerformanceFrequency(&iFrequency);
    QueryPerformanceCounter(&iCounter);
    return ((double)iCounter.QuadPart)/((double)iFrequency.QuadPart);
}
//----------------------------------------------------------------------------
