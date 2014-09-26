// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDBWideLookupControls.pas' rev: 6.00

#ifndef ElDBWideLookupControlsHPP
#define ElDBWideLookupControlsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <ElXPThemedControl.hpp>	// Pascal unit
#include <ElEdits.hpp>	// Pascal unit
#include <DBCtrls.hpp>	// Pascal unit
#include <DB.hpp>	// Pascal unit
#include <ElCombos.hpp>	// Pascal unit
#include <ElListBox.hpp>	// Pascal unit
#include <Dialogs.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Eldbwidelookupcontrols
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElWideDBLookupListControl;
class DELPHICLASS TElWideDBLookupListBox;
class PASCALIMPLEMENTATION TElWideDBLookupListBox : public Ellistbox::TElListBox 
{
	typedef Ellistbox::TElListBox inherited;
	
private:
	Classes::TNotifyEvent FOnChange;
	TElWideDBLookupListControl* FElDBWideLookupControl;
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall SetListSource(Db::TDataSource* Value);
	void __fastcall SetDataFieldName(const AnsiString Value);
	void __fastcall SetListFieldName(const AnsiString Value);
	void __fastcall SetKeyFieldName(const AnsiString Value);
	void __fastcall SetKeyValue(const Variant &Value);
	void __fastcall SetListFieldIndex(int Value);
	Db::TField* __fastcall GetField(void);
	int __fastcall GetListFieldIndex(void);
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TDataSource* __fastcall GetListSource(void);
	AnsiString __fastcall GetListFieldName();
	AnsiString __fastcall GetDataFieldName();
	AnsiString __fastcall GetKeyFieldName();
	Variant __fastcall GetKeyValue();
	WideString __fastcall GetSelectedString();
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TWMKey &Message);
	
protected:
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
public:
	__fastcall virtual TElWideDBLookupListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElWideDBLookupListBox(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	__property Variant KeyValue = {read=GetKeyValue, write=SetKeyValue};
	__property WideString SelectedItem = {read=GetSelectedString};
	__property int ListFieldIndex = {read=GetListFieldIndex, write=SetListFieldIndex, nodefault};
	__property Db::TField* Field = {read=GetField};
	
__published:
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property Db::TDataSource* ListSource = {read=GetListSource, write=SetListSource};
	__property AnsiString DataField = {read=GetDataFieldName, write=SetDataFieldName};
	__property AnsiString ListField = {read=GetListFieldName, write=SetListFieldName};
	__property AnsiString KeyField = {read=GetKeyFieldName, write=SetKeyFieldName};
	__property Classes::TNotifyEvent OnChange = {read=FOnChange, write=FOnChange};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElWideDBLookupListBox(HWND ParentWindow) : Ellistbox::TElListBox(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElWideDBLookupListControl : public Dbctrls::TDBLookupControl 
{
	typedef Dbctrls::TDBLookupControl inherited;
	
private:
	TElWideDBLookupListBox* FElDBWideLookupListBox;
	
protected:
	virtual void __fastcall KeyValueChanged(void);
	virtual void __fastcall UpdateListFields(void);
	void __fastcall SelectCurrent(void);
	void __fastcall Select(int Value);
	
public:
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
public:
	#pragma option push -w-inl
	/* TDBLookupControl.Create */ inline __fastcall virtual TElWideDBLookupListControl(Classes::TComponent* AOwner) : Dbctrls::TDBLookupControl(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TDBLookupControl.Destroy */ inline __fastcall virtual ~TElWideDBLookupListControl(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElWideDBLookupListControl(HWND ParentWindow) : Dbctrls::TDBLookupControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElWideDBLookUpComboControl;
class DELPHICLASS TElWideDBLookupComboBox;
class PASCALIMPLEMENTATION TElWideDBLookupComboBox : public Elcombos::TElComboBox 
{
	typedef Elcombos::TElComboBox inherited;
	
private:
	Classes::TWndMethod FSaveListWindowProc;
	TElWideDBLookUpComboControl* FElDBWideLookupControl;
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall SetListSource(Db::TDataSource* Value);
	void __fastcall SetDataFieldName(const AnsiString Value);
	void __fastcall SetListFieldName(const AnsiString Value);
	void __fastcall SetKeyFieldName(const AnsiString Value);
	void __fastcall SetKeyValue(const Variant &Value);
	void __fastcall SetListFieldIndex(int Value);
	void __fastcall SetSelected(int index, bool Value);
	bool __fastcall GetSelected(int index);
	Db::TField* __fastcall GetField(void);
	int __fastcall GetListFieldIndex(void);
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TDataSource* __fastcall GetListSource(void);
	AnsiString __fastcall GetListFieldName();
	AnsiString __fastcall GetDataFieldName();
	AnsiString __fastcall GetKeyFieldName();
	Variant __fastcall GetKeyValue();
	WideString __fastcall GetSelectedString();
	void __fastcall ListWindowProc(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TWMKey &Message);
	HIDESBASE MESSAGE void __fastcall WMKeyUp(Messages::TMessage &Message);
	HIDESBASE MESSAGE void __fastcall WMMouseMove(Messages::TWMMouse &Message);
	
public:
	__fastcall virtual TElWideDBLookupComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElWideDBLookupComboBox(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	__property Variant KeyValue = {read=GetKeyValue, write=SetKeyValue};
	__property WideString SelectedItem = {read=GetSelectedString};
	__property int ListFieldIndex = {read=GetListFieldIndex, write=SetListFieldIndex, nodefault};
	__property Db::TField* Field = {read=GetField};
	__property bool Selected[int Index] = {read=GetSelected, write=SetSelected};
	
__published:
	__property Db::TDataSource* DataSource = {read=GetDataSource, write=SetDataSource};
	__property Db::TDataSource* ListSource = {read=GetListSource, write=SetListSource};
	__property AnsiString DataField = {read=GetDataFieldName, write=SetDataFieldName};
	__property AnsiString ListField = {read=GetListFieldName, write=SetListFieldName};
	__property AnsiString KeyField = {read=GetKeyFieldName, write=SetKeyFieldName};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElWideDBLookupComboBox(HWND ParentWindow) : Elcombos::TElComboBox(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElWideDBLookUpComboControl : public Dbctrls::TDBLookupControl 
{
	typedef Dbctrls::TDBLookupControl inherited;
	
private:
	TElWideDBLookupComboBox* FElDBWideLookupComboBox;
	
protected:
	virtual void __fastcall KeyValueChanged(void);
	virtual void __fastcall UpdateListFields(void);
	void __fastcall SelectCurrent(void);
	void __fastcall Select(int Value);
	
public:
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
public:
	#pragma option push -w-inl
	/* TDBLookupControl.Create */ inline __fastcall virtual TElWideDBLookUpComboControl(Classes::TComponent* AOwner) : Dbctrls::TDBLookupControl(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TDBLookupControl.Destroy */ inline __fastcall virtual ~TElWideDBLookUpComboControl(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElWideDBLookUpComboControl(HWND ParentWindow) : Dbctrls::TDBLookupControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldbwidelookupcontrols */
using namespace Eldbwidelookupcontrols;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDBWideLookupControls
