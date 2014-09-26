// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElColor.pas' rev: 6.00

#ifndef ElColorHPP
#define ElColorHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTools.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcolor
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE int __fastcall ColorToRGB(const Graphics::TColor Color);
extern PACKAGE Graphics::TColor __fastcall BrightColor(const Graphics::TColor Color, const Byte Percent);
extern PACKAGE Graphics::TColor __fastcall DarkColor(const Graphics::TColor Color, const Byte Percent);
extern PACKAGE Graphics::TColor __fastcall ColorToGray(const Graphics::TColor Color);
extern PACKAGE int __fastcall ConvertColorToHTML(Graphics::TColor Color);

}	/* namespace Elcolor */
using namespace Elcolor;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElColor
