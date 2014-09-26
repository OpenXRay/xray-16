// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'MxPrgrss.pas' rev: 6.00

#ifndef MxPrgrssHPP
#define MxPrgrssHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxprgrss
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern PACKAGE bool __fastcall SupportsProgressControl(Controls::TControl* Control);
extern PACKAGE void __fastcall RegisterProgressControl(TMetaClass* AClass, const AnsiString MaxPropName, const AnsiString MinPropName, const AnsiString ProgressPropName);
extern PACKAGE void __fastcall UnRegisterProgressControl(TMetaClass* AClass);
extern PACKAGE void __fastcall SetProgressMax(Controls::TControl* Control, int MaxValue);
extern PACKAGE void __fastcall SetProgressMin(Controls::TControl* Control, int MinValue);
extern PACKAGE void __fastcall SetProgressValue(Controls::TControl* Control, int ProgressValue);

}	/* namespace Mxprgrss */
using namespace Mxprgrss;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// MxPrgrss
