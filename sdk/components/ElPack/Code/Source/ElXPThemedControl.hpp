// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElXPThemedControl.pas' rev: 6.00

#ifndef ElXPThemedControlHPP
#define ElXPThemedControlHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Messages.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <ElTmSchema.hpp>	// Pascal unit
#include <ElUxTheme.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elxpthemedcontrol
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElXPThemedControl;
class PASCALIMPLEMENTATION TElXPThemedControl : public Controls::TCustomControl 
{
	typedef Controls::TCustomControl inherited;
	
private:
	bool FUseXPThemes;
	unsigned FTheme;
	
protected:
	virtual void __fastcall SetUseXPThemes(const bool Value);
	virtual WideString __fastcall GetThemedClassName(void) = 0 ;
	virtual void __fastcall CreateWnd(void);
	virtual void __fastcall DestroyWnd(void);
	MESSAGE void __fastcall WMThemeChanged(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMNCPaint(Messages::TMessage &Message);
	virtual void __fastcall FreeThemeHandle(void);
	virtual void __fastcall CreateThemeHandle(void);
	__property bool UseXPThemes = {read=FUseXPThemes, write=SetUseXPThemes, default=1};
	
public:
	__fastcall virtual TElXPThemedControl(Classes::TComponent* AOwner);
	bool __fastcall IsThemeApplied(void);
	__property unsigned Theme = {read=FTheme, nodefault};
public:
	#pragma option push -w-inl
	/* TCustomControl.Destroy */ inline __fastcall virtual ~TElXPThemedControl(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElXPThemedControl(HWND ParentWindow) : Controls::TCustomControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elxpthemedcontrol */
using namespace Elxpthemedcontrol;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElXPThemedControl
