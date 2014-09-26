// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElCLabel.pas' rev: 6.00

#ifndef ElCLabelHPP
#define ElCLabelHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elclabel
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElCustomLabel;
class PASCALIMPLEMENTATION TElCustomLabel : public Stdctrls::TLabel 
{
	typedef Stdctrls::TLabel inherited;
	
public:
	virtual void __fastcall Loaded(void);
	
__published:
	__property Color ;
public:
	#pragma option push -w-inl
	/* TCustomLabel.Create */ inline __fastcall virtual TElCustomLabel(Classes::TComponent* AOwner) : Stdctrls::TLabel(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TGraphicControl.Destroy */ inline __fastcall virtual ~TElCustomLabel(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elclabel */
using namespace Elclabel;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElCLabel
