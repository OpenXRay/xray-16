// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHint.pas' rev: 6.00

#ifndef ElHintHPP
#define ElHintHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Controls.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhint
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElHintWindow;
class PASCALIMPLEMENTATION TElHintWindow : public Controls::THintWindow 
{
	typedef Controls::THintWindow inherited;
	
private:
	HIDESBASE MESSAGE void __fastcall WMNCPAINT(Messages::TMessage &Msg);
	HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &Msg);
	
protected:
	int XLoc;
	int YLoc;
	virtual void __fastcall Paint(void);
	
public:
	__fastcall virtual TElHintWindow(Classes::TComponent* AOwner);
	__fastcall virtual ~TElHintWindow(void);
	virtual void __fastcall ActivateHint(const Types::TRect &Rect, const AnsiString AHint);
	virtual Types::TRect __fastcall CalcHintRect(int MaxWidth, const AnsiString AHint, void * AData);
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElHintWindow(HWND ParentWindow) : Controls::THintWindow(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE void __fastcall SetHintWindow(void);

}	/* namespace Elhint */
using namespace Elhint;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHint
