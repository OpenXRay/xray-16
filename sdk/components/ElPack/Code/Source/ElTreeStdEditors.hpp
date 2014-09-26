// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElTreeStdEditors.pas' rev: 6.00

#ifndef ElTreeStdEditorsHPP
#define ElTreeStdEditorsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElHeader.hpp>	// Pascal unit
#include <ElTree.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <ImgList.hpp>	// Pascal unit
#include <Types.hpp>	// Pascal unit
#include <ComCtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eltreestdeditors
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElTreeInplaceEdit;
class PASCALIMPLEMENTATION TElTreeInplaceEdit : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Eledits::TElEdit* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceEdit(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceEdit(void);
	__property Eledits::TElEdit* Editor = {read=FEditor};
};


class DELPHICLASS TElTreeInplaceMemo;
class PASCALIMPLEMENTATION TElTreeInplaceMemo : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Stdctrls::TMemo* FEditor;
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceMemo(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceMemo(void);
	__property Stdctrls::TMemo* Editor = {read=FEditor};
};


class DELPHICLASS TElTreeInplaceDateTimePicker;
class PASCALIMPLEMENTATION TElTreeInplaceDateTimePicker : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Comctrls::TDateTimePicker* FEditor;
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceDateTimePicker(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceDateTimePicker(void);
	__property Comctrls::TDateTimePicker* Editor = {read=FEditor};
};


class DELPHICLASS TElTreeInplaceCheckBox;
class PASCALIMPLEMENTATION TElTreeInplaceCheckBox : public Eltree::TElTreeInplaceEditor 
{
	typedef Eltree::TElTreeInplaceEditor inherited;
	
private:
	Classes::TWndMethod SaveWndProc;
	void __fastcall EditorWndProc(Messages::TMessage &Message);
	
protected:
	Stdctrls::TCheckBox* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceCheckBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceCheckBox(void);
	__property Stdctrls::TCheckBox* Editor = {read=FEditor};
};


class DELPHICLASS THackInplaceComboBox;
class PASCALIMPLEMENTATION THackInplaceComboBox : public Stdctrls::TComboBox 
{
	typedef Stdctrls::TComboBox inherited;
	
__published:
	virtual void __fastcall ComboWndProc(Messages::TMessage &Message, HWND ComboWnd, void * ComboProc);
	
protected:
	DYNAMIC void __fastcall KeyPress(char &Key);
	HIDESBASE MESSAGE void __fastcall WMGetDlgCode(Messages::TWMNoParams &Message);
public:
	#pragma option push -w-inl
	/* TCustomComboBox.Create */ inline __fastcall virtual THackInplaceComboBox(Classes::TComponent* AOwner) : Stdctrls::TComboBox(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TCustomComboBox.Destroy */ inline __fastcall virtual ~THackInplaceComboBox(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall THackInplaceComboBox(HWND ParentWindow) : Stdctrls::TComboBox(ParentWindow) { }
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
	bool FInitiallyDropped;
	Stdctrls::TComboBox* FEditor;
	virtual void __fastcall DoStartOperation(void);
	virtual void __fastcall DoStopOperation(bool Accepted);
	virtual bool __fastcall GetVisible(void);
	virtual void __fastcall TriggerAfterOperation(bool &Accepted, bool &DefaultConversion);
	virtual void __fastcall TriggerBeforeOperation(bool &DefaultConversion);
	virtual void __fastcall SetEditorParent(void);
	
public:
	__fastcall virtual TElTreeInplaceComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElTreeInplaceComboBox(void);
	__property Stdctrls::TComboBox* Editor = {read=FEditor};
	
__published:
	__property bool InitiallyDropped = {read=FInitiallyDropped, write=FInitiallyDropped, nodefault};
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eltreestdeditors */
using namespace Eltreestdeditors;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElTreeStdEditors
