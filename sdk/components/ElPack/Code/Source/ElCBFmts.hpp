// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCBFmts.pas' rev: 6.00

#ifndef ElCBFmtsHPP
#define ElCBFmtsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <SysUtils.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcbfmts
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool __fastcall HasFormat(unsigned Handle, int Index);
extern PACKAGE AnsiString __fastcall GetFormatName(int AFormat);
extern PACKAGE int __fastcall GetFormatIndex(AnsiString FormatName);

}	/* namespace Elcbfmts */
using namespace Elcbfmts;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCBFmts
