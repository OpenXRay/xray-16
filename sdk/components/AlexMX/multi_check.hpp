// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'multi_check.pas' rev: 6.00

#ifndef multi_checkHPP
#define multi_checkHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <CommCtrl.hpp>	// Pascal unit
#include <ComCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Multi_check
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TMultiObjCheck;
class PASCALIMPLEMENTATION TMultiObjCheck : public Stdctrls::TCheckBox 
{
	typedef Stdctrls::TCheckBox inherited;
	
__published:
	void __fastcall ObjFirstInit(Stdctrls::TCheckBoxState chk);
	void __fastcall ObjNextInit(Stdctrls::TCheckBoxState chk);
	void __fastcall ObjApply(int &_to);
public:
	#pragma option push -w-inl
	/* TCustomCheckBox.Create */ inline __fastcall virtual TMultiObjCheck(Classes::TComponent* AOwner) : Stdctrls::TCheckBox(AOwner) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TMultiObjCheck(HWND ParentWindow) : Stdctrls::TCheckBox(ParentWindow) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TWinControl.Destroy */ inline __fastcall virtual ~TMultiObjCheck(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Multi_check */
using namespace Multi_check;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// multi_check
