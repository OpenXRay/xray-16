// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElClipMon.pas' rev: 6.00

#ifndef ElClipMonHPP
#define ElClipMonHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElBaseComp.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <ElCBFmts.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elclipmon
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElClipboardMonitor;
class PASCALIMPLEMENTATION TElClipboardMonitor : public Elbasecomp::TElBaseComponent 
{
	typedef Elbasecomp::TElBaseComponent inherited;
	
protected:
	HWND FPrevHandle;
	Classes::TStrings* FDataFormats;
	Classes::TNotifyEvent FOnChange;
	virtual void __fastcall WndProc(Messages::TMessage &Message);
	virtual void __fastcall DoSetEnabled(bool AEnabled);
	virtual void __fastcall TriggerChangeEvent(void);
	Classes::TStrings* __fastcall GetDataFormats(void);
	
public:
	__fastcall virtual ~TElClipboardMonitor(void);
	AnsiString __fastcall GetDataString(AnsiString Format);
	__property Classes::TStrings* DataFormats = {read=GetDataFormats};
	
__published:
	__property Enabled  = {default=0};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
public:
	#pragma option push -w-inl
	/* TElBaseComponent.Create */ inline __fastcall virtual TElClipboardMonitor(Classes::TComponent* AOwner) : Elbasecomp::TElBaseComponent(AOwner) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elclipmon */
using namespace Elclipmon;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElClipMon
