// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeSpinEdit.pas' rev: 6.00

#ifndef ElTreeSpinEditHPP
#define ElTreeSpinEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElSpin.hpp>	// Pascal unit
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

namespace Eltreespinedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElTreeInplaceSpinEdit;
class PASCALIMPLEMENTATION TElTreeInplaceSpinEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Elspin::TElSpinEdit* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	
public:
	__fastcall virtual TElTreeInplaceSpinEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceSpinEdit(void);
	__property Elspin::TElSpinEdit* Editor = {read=FEditor};
};


class DELPHICLASS TElTreeInplaceFloatSpinEdit;
class PASCALIMPLEMENTATION TElTreeInplaceFloatSpinEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Elspin::TElFloatSpinEdit* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	
public:
	__fastcall virtual TElTreeInplaceFloatSpinEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceFloatSpinEdit(void);
	__property Elspin::TElFloatSpinEdit* Editor = {read=FEditor};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreespinedit */
using namespace Eltreespinedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeSpinEdit
