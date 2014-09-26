// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxClipIcon.pas' rev: 6.00

#ifndef mxClipIconHPP
#define mxClipIconHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxclipicon
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE Word CF_ICON;
extern PACKAGE void __fastcall CopyIconToClipboard(Graphics::TIcon* Icon, Graphics::TColor BackColor);
extern PACKAGE void __fastcall AssignClipboardIcon(Graphics::TIcon* Icon);
extern PACKAGE Graphics::TIcon* __fastcall CreateIconFromClipboard(void);
extern PACKAGE void __fastcall GetIconSize(HICON Icon, int &W, int &H);
extern PACKAGE HICON __fastcall CreateRealSizeIcon(Graphics::TIcon* Icon);
extern PACKAGE void __fastcall DrawRealSizeIcon(Graphics::TCanvas* Canvas, Graphics::TIcon* Icon, int X, int Y);

}	/* namespace Mxclipicon */
using namespace Mxclipicon;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxClipIcon
