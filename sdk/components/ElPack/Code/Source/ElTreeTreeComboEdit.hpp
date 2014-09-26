// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeTreeComboEdit.pas' rev: 6.00

#ifndef ElTreeTreeComboEditHPP
#define ElTreeTreeComboEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElTreeCombo.hpp>	// Pascal unit
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

namespace Eltreetreecomboedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElTreeInplaceTreeComboEdit;
class PASCALIMPLEMENTATION TElTreeInplaceTreeComboEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	Classes::TWndMethod SaveTreeWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	void __fastcall TreeWndProc(Messages::TMessage &Message);
	
protected:
	Eltreecombo::TElTreeCombo* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceTreeComboEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceTreeComboEdit(void);
	__property Eltreecombo::TElTreeCombo* Editor = {read=FEditor};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreetreecomboedit */
using namespace Eltreetreecomboedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeTreeComboEdit
