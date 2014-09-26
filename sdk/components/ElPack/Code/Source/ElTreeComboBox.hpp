// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeComboBox.pas' rev: 6.00

#ifndef ElTreeComboBoxHPP
#define ElTreeComboBoxHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <StdCtrls.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <HTMLLbx.hpp>	// Pascal unit
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

namespace Eltreecombobox
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS THackInplaceComboBox;
class PASCALIMPLEMENTATION THackInplaceComboBox : public Htmllbx::TElHTMLComboBox 
{
	typedef Htmllbx::TElHTMLComboBox inherited;
	
__published:
	virtual void __fastcall ComboWndProc(Messages::TMessage &Message, HWND ComboWnd, void * ComboProc);
public:
	#pragma option push -w-inl
	/* TCustomElHTMLComboBox.Create */ inline __fastcall virtual THackInplaceComboBox(Classes::TComponent* AOwner) : Htmllbx::TElHTMLComboBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomElHTMLComboBox.Destroy */ inline __fastcall virtual ~THackInplaceComboBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall THackInplaceComboBox(HWND ParentWindow) : Htmllbx::TElHTMLComboBox(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElTreeInplaceComboBox;
class PASCALIMPLEMENTATION TElTreeInplaceComboBox : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Htmllbx::TElHTMLComboBox* FEditor;
	bool FInitiallyDropped;
	virtual void __fastcall SetEditorParent(void);
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	
public:
	__fastcall virtual TElTreeInplaceComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceComboBox(void);
	__property Htmllbx::TElHTMLComboBox* Editor = {read=FEditor};
	
__published:
	__property bool InitiallyDropped = {read=FInitiallyDropped, write=FInitiallyDropped, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreecombobox */
using namespace Eltreecombobox;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeComboBox
