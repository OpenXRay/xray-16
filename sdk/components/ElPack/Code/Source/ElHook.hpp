// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElHook.pas' rev: 6.00

#ifndef ElHookHPP
#define ElHookHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Classes.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Elhook
{
//-- type declarations -------------------------------------------------------
typedef void __fastcall (__closure *TElHookEvent)(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);

class DELPHICLASS TElHook;
class PASCALIMPLEMENTATION TElHook : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FDesignActive;
	TElHookEvent FOnBeforeProcess;
	TElHookEvent FOnAfterProcess;
	bool FActive;
	Controls::TControl* FControl;
	void __fastcall SetControl(Controls::TControl* newValue);
	void __fastcall SetActive(bool newValue);
	void __fastcall SetDesignActive(bool newValue);
	
protected:
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	virtual void __fastcall TriggerBeforeProcessEvent(Messages::TMessage &Msg, bool &Handled);
	virtual void __fastcall TriggerAfterProcessEvent(Messages::TMessage &Msg, bool &Handled);
	
public:
	void __fastcall HookControl(Controls::TWinControl* AControl);
	__fastcall virtual ~TElHook(void);
	
__published:
	__property bool Active = {read=FActive, write=SetActive, nodefault};
	__property bool DesignActive = {read=FDesignActive, write=SetDesignActive, nodefault};
	__property Controls::TControl* Control = {read=FControl, write=SetControl};
	__property TElHookEvent OnBeforeProcess = {read=FOnBeforeProcess, write=FOnBeforeProcess};
	__property TElHookEvent OnAfterProcess = {read=FOnAfterProcess, write=FOnAfterProcess};
public:
	#pragma option push -w-inl
	/* TComponent.Create */ inline __fastcall virtual TElHook(Classes::TComponent* AOwner) : Classes::TComponent(AOwner) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Elhook */
using namespace Elhook;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElHook
