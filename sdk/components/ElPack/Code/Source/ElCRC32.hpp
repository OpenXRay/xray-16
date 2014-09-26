// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCRC32.pas' rev: 6.00

#ifndef ElCRC32HPP
#define ElCRC32HPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Consts.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcrc32
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE int __fastcall CRCBuffer(int InitialCRC, void * Buffer, int BufLen);
extern PACKAGE int __fastcall CRCStr(AnsiString Str);
extern PACKAGE int __fastcall CRC32(int crc, const Byte c);

}	/* namespace Elcrc32 */
using namespace Elcrc32;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCRC32
