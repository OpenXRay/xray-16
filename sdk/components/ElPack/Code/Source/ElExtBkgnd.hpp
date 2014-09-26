// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElExtBkgnd.pas' rev: 6.00

#ifndef ElExtBkgndHPP
#define ElExtBkgndHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElVCLUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elextbkgnd
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE void __fastcall ExtDrawBkgnd2(HDC DC, HWND Wnd, const Types::TRect &R, const Types::TRect &DocRect, const Types::TPoint &Origin, Graphics::TColor FillColor, Graphics::TBitmap* SourceBitmap, Elvclutils::TElBkGndType DrawMode);
extern PACKAGE void __fastcall ExtDrawBkgnd(HDC DC, HWND Wnd, const Types::TRect &RectDoc, const Types::TRect &RectWindow, const Types::TRect &RectDC, const Types::TRect &RectOnDC, bool InvertedMode, Graphics::TColor FillColor, Graphics::TColor OverColor, bool DoBlend, Graphics::TBitmap* SourceBitmap, Elvclutils::TElBkGndType DrawMode);

}	/* namespace Elextbkgnd */
using namespace Elextbkgnd;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElExtBkgnd
