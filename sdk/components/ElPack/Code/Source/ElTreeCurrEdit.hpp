// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeCurrEdit.pas' rev: 6.00

#ifndef ElTreeCurrEditHPP
#define ElTreeCurrEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElCurrEdit.hpp>	// Pascal unit
#include <ElStrUtils.hpp>	// Pascal unit
#include <ElTools.hpp>	// Pascal unit
#include <ElHeader.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreecurredit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS THackInplaceCurrencyEdit;
class PASCALIMPLEMENTATION THackInplaceCurrencyEdit : public Elcurredit::TElCurrencyEdit 
{
	typedef Elcurredit::TElCurrencyEdit inherited;
	
__published:
	DYNAMIC void __fastcall DoExit(void);
	DYNAMIC void __fastcall KeyDown(Word &Key, Classes::TShiftState ShiftState);
public:
	#pragma option push -w-inl
	/* TElCurrencyEdit.Create */ inline __fastcall virtual THackInplaceCurrencyEdit(Classes::TComponent* AOwner) : Elcurredit::TElCurrencyEdit(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TElCurrencyEdit.Destroy */ inline __fastcall virtual ~THackInplaceCurrencyEdit(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall THackInplaceCurrencyEdit(HWND ParentWindow) : Elcurredit::TElCurrencyEdit(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeInplaceCurrencyEdit;
class PASCALIMPLEMENTATION TElTreeInplaceCurrencyEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveIntWndProc[2];
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	void __fastcall IntEditorWndProc2(Messages::TMessage &Message);
	void __fastcall IntEditorWndProc1(Messages::TMessage &Message);
	
protected:
	Elcurredit::TElCurrencyEdit* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall SetEditorParent(void);
	void __fastcall RealWndProc(Messages::TMessage &Message);
	
public:
	__fastcall virtual TElTreeInplaceCurrencyEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceCurrencyEdit(void);
	__property Elcurredit::TElCurrencyEdit* Editor = {read=FEditor};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreecurredit */
using namespace Eltreecurredit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeCurrEdit
