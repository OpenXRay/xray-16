// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'mxHook.pas' rev: 6.00

#ifndef mxHookHPP
#define mxHookHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <mxConst.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Mxhook
{
//-- type declarations -------------------------------------------------------
typedef TMetaClass* *PClass;

typedef void __fastcall (__closure *THookMessageEvent)(System::TObject* Sender, Messages::TMessage &Msg, bool &Handled);

class DELPHICLASS TRxWindowHook;
class PASCALIMPLEMENTATION TRxWindowHook : public Classes::TComponent 
{
	typedef Classes::TComponent inherited;
	
private:
	bool FActive;
	Controls::TWinControl* FControl;
	System::TObject* FControlHook;
	THookMessageEvent FBeforeMessage;
	THookMessageEvent FAfterMessage;
	Controls::TWinControl* __fastcall GetWinControl(void);
	HWND __fastcall GetHookHandle(void);
	void __fastcall SetActive(bool Value);
	void __fastcall SetWinControl(Controls::TWinControl* Value);
	bool __fastcall IsForm(void);
	bool __fastcall NotIsForm(void);
	void * __fastcall DoUnhookControl(void);
	void __fastcall ReadForm(Classes::TReader* Reader);
	void __fastcall WriteForm(Classes::TWriter* Writer);
	
protected:
	virtual void __fastcall DefineProperties(Classes::TFiler* Filer);
	DYNAMIC void __fastcall DoAfterMessage(Messages::TMessage &Msg, bool &Handled);
	DYNAMIC void __fastcall DoBeforeMessage(Messages::TMessage &Msg, bool &Handled);
	virtual void __fastcall Notification(Classes::TComponent* AComponent, Classes::TOperation Operation);
	
public:
	__fastcall virtual TRxWindowHook(Classes::TComponent* AOwner);
	__fastcall virtual ~TRxWindowHook(void);
	void __fastcall HookControl(void);
	void __fastcall UnhookControl(void);
	__property HWND HookWindow = {read=GetHookHandle, nodefault};
	
__published:
	__property bool Active = {read=FActive, write=SetActive, default=1};
	__property Controls::TWinControl* WinControl = {read=GetWinControl, write=SetWinControl, stored=NotIsForm};
	__property THookMessageEvent BeforeMessage = {read=FBeforeMessage, write=FBeforeMessage};
	__property THookMessageEvent AfterMessage = {read=FAfterMessage, write=FAfterMessage};
};


//-- var, const, procedure ---------------------------------------------------
extern PACKAGE void * __fastcall GetVirtualMethodAddress(TMetaClass* AClass, int AIndex);
extern PACKAGE void * __fastcall SetVirtualMethodAddress(TMetaClass* AClass, int AIndex, void * NewAddress);
extern PACKAGE int __fastcall FindVirtualMethodIndex(TMetaClass* AClass, void * MethodAddr);

}	/* namespace Mxhook */
using namespace Mxhook;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// mxHook
