// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'ElDBLookupCtrls.pas' rev: 6.00

#ifndef ElDBLookupCtrlsHPP
#define ElDBLookupCtrlsHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <DBCtrls.hpp>	// Pascal unit
#include <DB.hpp>	// Pascal unit
#include <ElACtrls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
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

namespace Eldblookupctrls
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TElDBLookupListControl;
class DELPHICLASS TElDBLookupListBox;
class PASCALIMPLEMENTATION TElDBLookupListBox : public Elactrls::TElAdvancedListBox 
{
	typedef Elactrls::TElAdvancedListBox inherited;
	
private:
	TElDBLookupListControl* FElDBLookupControl;
	Classes::TNotifyEvent FOnChange;
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
	AnsiString __fastcall GetSelectedString();
	HIDESBASE MESSAGE void __fastcall WMLButtonDown(Messages::TWMMouse &Message);
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TWMKey &Message);
	
protected:
	DYNAMIC void __fastcall MouseDown(Controls::TMouseButton Button, Classes::TShiftState Shift, int X, int Y);
	
public:
	__fastcall virtual TElDBLookupListBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBLookupListBox(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	__property Variant KeyValue = {read=GetKeyValue, write=SetKeyValue};
	__property AnsiString SelectedItem = {read=GetSelectedString};
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
	/* TWinControl.CreateParented */ inline __fastcall TElDBLookupListBox(HWND ParentWindow) : Elactrls::TElAdvancedListBox(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElDBLookupListControl : public Dbctrls::TDBLookupControl 
{
	typedef Dbctrls::TDBLookupControl inherited;
	
private:
	TElDBLookupListBox* FElDBLookupListBox;
	
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
	/* TDBLookupControl.Create */ inline __fastcall virtual TElDBLookupListControl(Classes::TComponent* AOwner) : Dbctrls::TDBLookupControl(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TDBLookupControl.Destroy */ inline __fastcall virtual ~TElDBLookupListControl(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBLookupListControl(HWND ParentWindow) : Dbctrls::TDBLookupControl(ParentWindow) { }
	#pragma option pop
	
};


class DELPHICLASS TElDBLookUpComboControl;
class DELPHICLASS TElDBLookupComboBox;
typedef DynamicArray<bool >  ElDBLookupCtrls__5;

class PASCALIMPLEMENTATION TElDBLookupComboBox : public Elactrls::TElAdvancedComboBox 
{
	typedef Elactrls::TElAdvancedComboBox inherited;
	
private:
	TElDBLookUpComboControl* FElDBLookupControl;
	DynamicArray<bool >  FSelected;
	int FMaxItems;
	void __fastcall SetDataSource(Db::TDataSource* Value);
	void __fastcall SetListSource(Db::TDataSource* Value);
	void __fastcall SetDataFieldName(const AnsiString Value);
	void __fastcall SetListFieldName(const AnsiString Value);
	void __fastcall SetKeyFieldName(const AnsiString Value);
	void __fastcall SetKeyValue(const Variant &Value);
	void __fastcall SetListFieldIndex(int Value);
	void __fastcall SetSelected(int index, bool Value);
	HIDESBASE void __fastcall AddItem(const AnsiString Value);
	bool __fastcall GetSelected(int index);
	Db::TField* __fastcall GetField(void);
	int __fastcall GetListFieldIndex(void);
	Db::TDataSource* __fastcall GetDataSource(void);
	Db::TDataSource* __fastcall GetListSource(void);
	AnsiString __fastcall GetListFieldName();
	AnsiString __fastcall GetDataFieldName();
	AnsiString __fastcall GetKeyFieldName();
	Variant __fastcall GetKeyValue();
	AnsiString __fastcall GetSelectedString();
	HIDESBASE MESSAGE void __fastcall WMChar(Messages::TWMKey &Message);
	
protected:
	virtual void __fastcall EditWndProc(Messages::TMessage &Message);
	virtual void __fastcall ListWndProc(Messages::TMessage &Message);
	virtual void __fastcall DrawItem(int Index, const Types::TRect &Rect, Windows::TOwnerDrawState State);
	
public:
	__fastcall virtual TElDBLookupComboBox(Classes::TComponent* AOwner);
	__fastcall virtual ~TElDBLookupComboBox(void);
	DYNAMIC bool __fastcall ExecuteAction(Classes::TBasicAction* Action);
	DYNAMIC bool __fastcall UpdateAction(Classes::TBasicAction* Action);
	__property Variant KeyValue = {read=GetKeyValue, write=SetKeyValue};
	__property AnsiString SelectedItem = {read=GetSelectedString};
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
	/* TWinControl.CreateParented */ inline __fastcall TElDBLookupComboBox(HWND ParentWindow) : Elactrls::TElAdvancedComboBox(ParentWindow) { }
	#pragma option pop
	
};


class PASCALIMPLEMENTATION TElDBLookUpComboControl : public Dbctrls::TDBLookupControl 
{
	typedef Dbctrls::TDBLookupControl inherited;
	
private:
	TElDBLookupComboBox* FElDBLookupComboBox;
	
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
	/* TDBLookupControl.Create */ inline __fastcall virtual TElDBLookUpComboControl(Classes::TComponent* AOwner) : Dbctrls::TDBLookupControl(AOwner) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TDBLookupControl.Destroy */ inline __fastcall virtual ~TElDBLookUpComboControl(void) { }
	#pragma option pop
	
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TElDBLookUpComboControl(HWND ParentWindow) : Dbctrls::TDBLookupControl(ParentWindow) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------

}	/* namespace Eldblookupctrls */
using namespace Eldblookupctrls;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// ElDBLookupCtrls
