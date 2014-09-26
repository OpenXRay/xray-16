// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxMaxMin.pas' rev: 6.00

#ifndef mxMaxMinHPP
#define mxMaxMinHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxmaxmin
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE void __fastcall SwapInt(int &Int1, int &Int2);
extern PACKAGE void __fastcall SwapInt64(__int64 &Int1, __int64 &Int2);
extern PACKAGE void __fastcall SwapLong(int &Int1, int &Int2);
extern PACKAGE int __fastcall Max(int A, int B);
extern PACKAGE int __fastcall Min(int A, int B);
extern PACKAGE int __fastcall MaxInteger(const int * Values, const int Values_Size);
extern PACKAGE int __fastcall MinInteger(const int * Values, const int Values_Size);
extern PACKAGE __int64 __fastcall MaxInt64(const __int64 * Values, const int Values_Size);
extern PACKAGE __int64 __fastcall MinInt64(const __int64 * Values, const int Values_Size);
extern PACKAGE Extended __fastcall MaxFloat(const Extended * Values, const int Values_Size);
extern PACKAGE Extended __fastcall MinFloat(const Extended * Values, const int Values_Size);
extern PACKAGE System::TDateTime __fastcall MaxDateTime(const System::TDateTime * Values, const int Values_Size);
extern PACKAGE System::TDateTime __fastcall MinDateTime(const System::TDateTime * Values, const int Values_Size);
extern PACKAGE Variant __fastcall MaxOf(const Variant * Values, const int Values_Size);
extern PACKAGE Variant __fastcall MinOf(const Variant * Values, const int Values_Size);

}	/* namespace Mxmaxmin */
using namespace Mxmaxmin;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxMaxMin
