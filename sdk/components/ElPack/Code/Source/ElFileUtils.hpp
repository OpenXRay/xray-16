// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElFileUtils.pas' rev: 6.00

#ifndef ElFileUtilsHPP
#define ElFileUtilsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elfileutils
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE WideString __fastcall GetWideCurrentDir();
extern PACKAGE bool __fastcall SetWideCurrentDir(const WideString Dir);
extern PACKAGE bool __fastcall CreateWideDir(const WideString Dir);
extern PACKAGE bool __fastcall RemoveWideDir(const WideString Dir);
extern PACKAGE WideString __fastcall GetWideModuleName(unsigned Module);

}	/* namespace Elfileutils */
using namespace Elfileutils;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElFileUtils
