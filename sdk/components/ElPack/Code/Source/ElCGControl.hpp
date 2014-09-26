// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCGControl.pas' rev: 6.00

#ifndef ElCGControlHPP
#define ElCGControlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elcgcontrol
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCustomGraphicControl;
class PASCALIMPLEMENTATION TElCustomGraphicControl : public Controls::TGraphicControl 
{
	typedef Controls::TGraphicControl inherited;
	
public:
	virtual void __fastcall Loaded(void);
	
__published:
	__property Color ;
public:
	#pragma option push -w-inl
	/* TGraphicControl.Create */ inline __fastcall virtual TElCustomGraphicControl(Classes::TComponent* AOwner) : Controls::TGraphicControl(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TGraphicControl.Destroy */ inline __fastcall virtual ~TElCustomGraphicControl(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elcgcontrol */
using namespace Elcgcontrol;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCGControl
