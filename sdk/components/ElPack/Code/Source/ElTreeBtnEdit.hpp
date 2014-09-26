// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeBtnEdit.pas' rev: 6.00

#ifndef ElTreeBtnEditHPP
#define ElTreeBtnEditHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElBtnEdit.hpp>	// Pascal unit
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

namespace Eltreebtnedit
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElTreeInplaceButtonEdit;
class PASCALIMPLEMENTATION TElTreeInplaceButtonEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Elbtnedit::TElButtonEdit* FEditor;
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceButtonEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceButtonEdit(void);
	__property Elbtnedit::TElButtonEdit* Editor = {read=FEditor};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreebtnedit */
using namespace Eltreebtnedit;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeBtnEdit
